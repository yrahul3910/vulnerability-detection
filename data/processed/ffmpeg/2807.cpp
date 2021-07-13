static int vc1_decode_p_mb_intfr(VC1Context *v)

{

    MpegEncContext *s = &v->s;

    GetBitContext *gb = &s->gb;

    int i;

    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;

    int cbp = 0; /* cbp decoding stuff */

    int mqdiff, mquant; /* MB quantization */

    int ttmb = v->ttfrm; /* MB Transform type */



    int mb_has_coeffs = 1; /* last_flag */

    int dmv_x, dmv_y; /* Differential MV components */

    int val; /* temp value */

    int first_block = 1;

    int dst_idx, off;

    int skipped, fourmv = 0, twomv = 0;

    int block_cbp = 0, pat, block_tt = 0;

    int idx_mbmode = 0, mvbp;

    int stride_y, fieldtx;



    mquant = v->pq; /* Lossy initialization */



    if (v->skip_is_raw)

        skipped = get_bits1(gb);

    else

        skipped = v->s.mbskip_table[mb_pos];

    if (!skipped) {

        if (v->fourmvswitch)

            idx_mbmode = get_vlc2(gb, v->mbmode_vlc->table, VC1_INTFR_4MV_MBMODE_VLC_BITS, 2); // try getting this done

        else

            idx_mbmode = get_vlc2(gb, v->mbmode_vlc->table, VC1_INTFR_NON4MV_MBMODE_VLC_BITS, 2); // in a single line

        switch (ff_vc1_mbmode_intfrp[v->fourmvswitch][idx_mbmode][0]) {

        /* store the motion vector type in a flag (useful later) */

        case MV_PMODE_INTFR_4MV:

            fourmv = 1;

            v->blk_mv_type[s->block_index[0]] = 0;

            v->blk_mv_type[s->block_index[1]] = 0;

            v->blk_mv_type[s->block_index[2]] = 0;

            v->blk_mv_type[s->block_index[3]] = 0;

            break;

        case MV_PMODE_INTFR_4MV_FIELD:

            fourmv = 1;

            v->blk_mv_type[s->block_index[0]] = 1;

            v->blk_mv_type[s->block_index[1]] = 1;

            v->blk_mv_type[s->block_index[2]] = 1;

            v->blk_mv_type[s->block_index[3]] = 1;

            break;

        case MV_PMODE_INTFR_2MV_FIELD:

            twomv = 1;

            v->blk_mv_type[s->block_index[0]] = 1;

            v->blk_mv_type[s->block_index[1]] = 1;

            v->blk_mv_type[s->block_index[2]] = 1;

            v->blk_mv_type[s->block_index[3]] = 1;

            break;

        case MV_PMODE_INTFR_1MV:

            v->blk_mv_type[s->block_index[0]] = 0;

            v->blk_mv_type[s->block_index[1]] = 0;

            v->blk_mv_type[s->block_index[2]] = 0;

            v->blk_mv_type[s->block_index[3]] = 0;

            break;

        }

        if (ff_vc1_mbmode_intfrp[v->fourmvswitch][idx_mbmode][0] == MV_PMODE_INTFR_INTRA) { // intra MB

            for (i = 0; i < 4; i++) {

                s->current_picture.motion_val[1][s->block_index[i]][0] = 0;

                s->current_picture.motion_val[1][s->block_index[i]][1] = 0;

            }

            v->is_intra[s->mb_x] = 0x3f; // Set the bitfield to all 1.

            s->mb_intra          = 1;

            s->current_picture.mb_type[mb_pos] = MB_TYPE_INTRA;

            fieldtx = v->fieldtx_plane[mb_pos] = get_bits1(gb);

            mb_has_coeffs = get_bits1(gb);

            if (mb_has_coeffs)

                cbp = 1 + get_vlc2(&v->s.gb, v->cbpcy_vlc->table, VC1_CBPCY_P_VLC_BITS, 2);

            v->s.ac_pred = v->acpred_plane[mb_pos] = get_bits1(gb);

            GET_MQUANT();

            s->current_picture.qscale_table[mb_pos] = mquant;

            /* Set DC scale - y and c use the same (not sure if necessary here) */

            s->y_dc_scale = s->y_dc_scale_table[mquant];

            s->c_dc_scale = s->c_dc_scale_table[mquant];

            dst_idx = 0;

            for (i = 0; i < 6; i++) {

                v->a_avail = v->c_avail          = 0;

                v->mb_type[0][s->block_index[i]] = 1;

                s->dc_val[0][s->block_index[i]]  = 0;

                dst_idx += i >> 2;

                val = ((cbp >> (5 - i)) & 1);

                if (i == 2 || i == 3 || !s->first_slice_line)

                    v->a_avail = v->mb_type[0][s->block_index[i] - s->block_wrap[i]];

                if (i == 1 || i == 3 || s->mb_x)

                    v->c_avail = v->mb_type[0][s->block_index[i] - 1];



                vc1_decode_intra_block(v, s->block[i], i, val, mquant,

                                       (i & 4) ? v->codingset2 : v->codingset);

                if ((i>3) && (s->flags & CODEC_FLAG_GRAY)) continue;

                v->vc1dsp.vc1_inv_trans_8x8(s->block[i]);

                if (i < 4) {

                    stride_y = s->linesize << fieldtx;

                    off = (fieldtx) ? ((i & 1) * 8) + ((i & 2) >> 1) * s->linesize : (i & 1) * 8 + 4 * (i & 2) * s->linesize;

                } else {

                    stride_y = s->uvlinesize;

                    off = 0;

                }

                s->idsp.put_signed_pixels_clamped(s->block[i],

                                                  s->dest[dst_idx] + off,

                                                  stride_y);

                //TODO: loop filter

            }



        } else { // inter MB

            mb_has_coeffs = ff_vc1_mbmode_intfrp[v->fourmvswitch][idx_mbmode][3];

            if (mb_has_coeffs)

                cbp = 1 + get_vlc2(&v->s.gb, v->cbpcy_vlc->table, VC1_CBPCY_P_VLC_BITS, 2);

            if (ff_vc1_mbmode_intfrp[v->fourmvswitch][idx_mbmode][0] == MV_PMODE_INTFR_2MV_FIELD) {

                v->twomvbp = get_vlc2(gb, v->twomvbp_vlc->table, VC1_2MV_BLOCK_PATTERN_VLC_BITS, 1);

            } else {

                if ((ff_vc1_mbmode_intfrp[v->fourmvswitch][idx_mbmode][0] == MV_PMODE_INTFR_4MV)

                    || (ff_vc1_mbmode_intfrp[v->fourmvswitch][idx_mbmode][0] == MV_PMODE_INTFR_4MV_FIELD)) {

                    v->fourmvbp = get_vlc2(gb, v->fourmvbp_vlc->table, VC1_4MV_BLOCK_PATTERN_VLC_BITS, 1);

                }

            }

            s->mb_intra = v->is_intra[s->mb_x] = 0;

            for (i = 0; i < 6; i++)

                v->mb_type[0][s->block_index[i]] = 0;

            fieldtx = v->fieldtx_plane[mb_pos] = ff_vc1_mbmode_intfrp[v->fourmvswitch][idx_mbmode][1];

            /* for all motion vector read MVDATA and motion compensate each block */

            dst_idx = 0;

            if (fourmv) {

                mvbp = v->fourmvbp;

                for (i = 0; i < 6; i++) {

                    if (i < 4) {

                        dmv_x = dmv_y = 0;

                        val   = ((mvbp >> (3 - i)) & 1);

                        if (val) {

                            get_mvdata_interlaced(v, &dmv_x, &dmv_y, 0);

                        }

                        ff_vc1_pred_mv_intfr(v, i, dmv_x, dmv_y, 0, v->range_x, v->range_y, v->mb_type[0], 0);

                        ff_vc1_mc_4mv_luma(v, i, 0, 0);

                    } else if (i == 4) {

                        ff_vc1_mc_4mv_chroma4(v, 0, 0, 0);

                    }

                }

            } else if (twomv) {

                mvbp  = v->twomvbp;

                dmv_x = dmv_y = 0;

                if (mvbp & 2) {

                    get_mvdata_interlaced(v, &dmv_x, &dmv_y, 0);

                }

                ff_vc1_pred_mv_intfr(v, 0, dmv_x, dmv_y, 2, v->range_x, v->range_y, v->mb_type[0], 0);

                ff_vc1_mc_4mv_luma(v, 0, 0, 0);

                ff_vc1_mc_4mv_luma(v, 1, 0, 0);

                dmv_x = dmv_y = 0;

                if (mvbp & 1) {

                    get_mvdata_interlaced(v, &dmv_x, &dmv_y, 0);

                }

                ff_vc1_pred_mv_intfr(v, 2, dmv_x, dmv_y, 2, v->range_x, v->range_y, v->mb_type[0], 0);

                ff_vc1_mc_4mv_luma(v, 2, 0, 0);

                ff_vc1_mc_4mv_luma(v, 3, 0, 0);

                ff_vc1_mc_4mv_chroma4(v, 0, 0, 0);

            } else {

                mvbp = ff_vc1_mbmode_intfrp[v->fourmvswitch][idx_mbmode][2];

                dmv_x = dmv_y = 0;

                if (mvbp) {

                    get_mvdata_interlaced(v, &dmv_x, &dmv_y, 0);

                }

                ff_vc1_pred_mv_intfr(v, 0, dmv_x, dmv_y, 1, v->range_x, v->range_y, v->mb_type[0], 0);

                ff_vc1_mc_1mv(v, 0);

            }

            if (cbp)

                GET_MQUANT();  // p. 227

            s->current_picture.qscale_table[mb_pos] = mquant;

            if (!v->ttmbf && cbp)

                ttmb = get_vlc2(gb, ff_vc1_ttmb_vlc[v->tt_index].table, VC1_TTMB_VLC_BITS, 2);

            for (i = 0; i < 6; i++) {

                s->dc_val[0][s->block_index[i]] = 0;

                dst_idx += i >> 2;

                val = ((cbp >> (5 - i)) & 1);

                if (!fieldtx)

                    off = (i & 4) ? 0 : ((i & 1) * 8 + (i & 2) * 4 * s->linesize);

                else

                    off = (i & 4) ? 0 : ((i & 1) * 8 + ((i > 1) * s->linesize));

                if (val) {

                    pat = vc1_decode_p_block(v, s->block[i], i, mquant, ttmb,

                                             first_block, s->dest[dst_idx] + off,

                                             (i & 4) ? s->uvlinesize : (s->linesize << fieldtx),

                                             (i & 4) && (s->flags & CODEC_FLAG_GRAY), &block_tt);

                    block_cbp |= pat << (i << 2);

                    if (!v->ttmbf && ttmb < 8)

                        ttmb = -1;

                    first_block = 0;

                }

            }

        }

    } else { // skipped

        s->mb_intra = v->is_intra[s->mb_x] = 0;

        for (i = 0; i < 6; i++) {

            v->mb_type[0][s->block_index[i]] = 0;

            s->dc_val[0][s->block_index[i]] = 0;

        }

        s->current_picture.mb_type[mb_pos]      = MB_TYPE_SKIP;

        s->current_picture.qscale_table[mb_pos] = 0;

        v->blk_mv_type[s->block_index[0]] = 0;

        v->blk_mv_type[s->block_index[1]] = 0;

        v->blk_mv_type[s->block_index[2]] = 0;

        v->blk_mv_type[s->block_index[3]] = 0;

        ff_vc1_pred_mv_intfr(v, 0, 0, 0, 1, v->range_x, v->range_y, v->mb_type[0], 0);

        ff_vc1_mc_1mv(v, 0);

    }

    if (s->mb_x == s->mb_width - 1)

        memmove(v->is_intra_base, v->is_intra, sizeof(v->is_intra_base[0])*s->mb_stride);

    return 0;

}
