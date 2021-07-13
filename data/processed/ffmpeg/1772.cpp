static int hls_slice_header(HEVCContext *s)

{

    GetBitContext *gb = &s->HEVClc->gb;

    SliceHeader *sh   = &s->sh;

    int i, j, ret;



    // Coded parameters

    sh->first_slice_in_pic_flag = get_bits1(gb);

    if ((IS_IDR(s) || IS_BLA(s)) && sh->first_slice_in_pic_flag) {

        s->seq_decode = (s->seq_decode + 1) & 0xff;

        s->max_ra     = INT_MAX;

        if (IS_IDR(s))

            ff_hevc_clear_refs(s);

    }

    sh->no_output_of_prior_pics_flag = 0;

    if (IS_IRAP(s))

        sh->no_output_of_prior_pics_flag = get_bits1(gb);



    sh->pps_id = get_ue_golomb_long(gb);

    if (sh->pps_id >= MAX_PPS_COUNT || !s->pps_list[sh->pps_id]) {

        av_log(s->avctx, AV_LOG_ERROR, "PPS id out of range: %d\n", sh->pps_id);

        return AVERROR_INVALIDDATA;

    }

    if (!sh->first_slice_in_pic_flag &&

        s->pps != (HEVCPPS*)s->pps_list[sh->pps_id]->data) {

        av_log(s->avctx, AV_LOG_ERROR, "PPS changed between slices.\n");

        return AVERROR_INVALIDDATA;

    }

    s->pps = (HEVCPPS*)s->pps_list[sh->pps_id]->data;

    if (s->nal_unit_type == NAL_CRA_NUT && s->last_eos == 1)

        sh->no_output_of_prior_pics_flag = 1;



    if (s->sps != (HEVCSPS*)s->sps_list[s->pps->sps_id]->data) {

        const HEVCSPS* last_sps = s->sps;

        s->sps = (HEVCSPS*)s->sps_list[s->pps->sps_id]->data;

        if (last_sps && IS_IRAP(s) && s->nal_unit_type != NAL_CRA_NUT) {

            if (s->sps->width !=  last_sps->width || s->sps->height != last_sps->height ||

                s->sps->temporal_layer[s->sps->max_sub_layers - 1].max_dec_pic_buffering !=

                last_sps->temporal_layer[last_sps->max_sub_layers - 1].max_dec_pic_buffering)

                sh->no_output_of_prior_pics_flag = 0;

        }

        ff_hevc_clear_refs(s);

        ret = set_sps(s, s->sps, AV_PIX_FMT_NONE);

        if (ret < 0)

            return ret;



        s->seq_decode = (s->seq_decode + 1) & 0xff;

        s->max_ra     = INT_MAX;

    }



    sh->dependent_slice_segment_flag = 0;

    if (!sh->first_slice_in_pic_flag) {

        int slice_address_length;



        if (s->pps->dependent_slice_segments_enabled_flag)

            sh->dependent_slice_segment_flag = get_bits1(gb);



        slice_address_length = av_ceil_log2(s->sps->ctb_width *

                                            s->sps->ctb_height);

        sh->slice_segment_addr = get_bits(gb, slice_address_length);

        if (sh->slice_segment_addr >= s->sps->ctb_width * s->sps->ctb_height) {

            av_log(s->avctx, AV_LOG_ERROR,

                   "Invalid slice segment address: %u.\n",

                   sh->slice_segment_addr);

            return AVERROR_INVALIDDATA;

        }



        if (!sh->dependent_slice_segment_flag) {

            sh->slice_addr = sh->slice_segment_addr;

            s->slice_idx++;

        }

    } else {

        sh->slice_segment_addr = sh->slice_addr = 0;

        s->slice_idx           = 0;

        s->slice_initialized   = 0;

    }



    if (!sh->dependent_slice_segment_flag) {

        s->slice_initialized = 0;



        for (i = 0; i < s->pps->num_extra_slice_header_bits; i++)

            skip_bits(gb, 1);  // slice_reserved_undetermined_flag[]



        sh->slice_type = get_ue_golomb_long(gb);

        if (!(sh->slice_type == I_SLICE ||

              sh->slice_type == P_SLICE ||

              sh->slice_type == B_SLICE)) {

            av_log(s->avctx, AV_LOG_ERROR, "Unknown slice type: %d.\n",

                   sh->slice_type);

            return AVERROR_INVALIDDATA;

        }

        if (IS_IRAP(s) && sh->slice_type != I_SLICE) {

            av_log(s->avctx, AV_LOG_ERROR, "Inter slices in an IRAP frame.\n");

            return AVERROR_INVALIDDATA;

        }



        // when flag is not present, picture is inferred to be output

        sh->pic_output_flag = 1;

        if (s->pps->output_flag_present_flag)

            sh->pic_output_flag = get_bits1(gb);



        if (s->sps->separate_colour_plane_flag)

            sh->colour_plane_id = get_bits(gb, 2);



        if (!IS_IDR(s)) {

            int poc;



            sh->pic_order_cnt_lsb = get_bits(gb, s->sps->log2_max_poc_lsb);

            poc = ff_hevc_compute_poc(s, sh->pic_order_cnt_lsb);

            if (!sh->first_slice_in_pic_flag && poc != s->poc) {

                av_log(s->avctx, AV_LOG_WARNING,

                       "Ignoring POC change between slices: %d -> %d\n", s->poc, poc);

                if (s->avctx->err_recognition & AV_EF_EXPLODE)

                    return AVERROR_INVALIDDATA;

                poc = s->poc;

            }

            s->poc = poc;



            sh->short_term_ref_pic_set_sps_flag = get_bits1(gb);

            if (!sh->short_term_ref_pic_set_sps_flag) {

                int pos = get_bits_left(gb);

                ret = ff_hevc_decode_short_term_rps(s, &sh->slice_rps, s->sps, 1);

                if (ret < 0)

                    return ret;



                sh->short_term_ref_pic_set_size = pos - get_bits_left(gb);

                sh->short_term_rps = &sh->slice_rps;

            } else {

                int numbits, rps_idx;



                if (!s->sps->nb_st_rps) {

                    av_log(s->avctx, AV_LOG_ERROR, "No ref lists in the SPS.\n");

                    return AVERROR_INVALIDDATA;

                }



                numbits = av_ceil_log2(s->sps->nb_st_rps);

                rps_idx = numbits > 0 ? get_bits(gb, numbits) : 0;

                sh->short_term_rps = &s->sps->st_rps[rps_idx];

            }



            ret = decode_lt_rps(s, &sh->long_term_rps, gb);

            if (ret < 0) {

                av_log(s->avctx, AV_LOG_WARNING, "Invalid long term RPS.\n");

                if (s->avctx->err_recognition & AV_EF_EXPLODE)

                    return AVERROR_INVALIDDATA;

            }



            if (s->sps->sps_temporal_mvp_enabled_flag)

                sh->slice_temporal_mvp_enabled_flag = get_bits1(gb);

            else

                sh->slice_temporal_mvp_enabled_flag = 0;

        } else {

            s->sh.short_term_rps = NULL;

            s->poc               = 0;

        }



        /* 8.3.1 */

        if (s->temporal_id == 0 &&

            s->nal_unit_type != NAL_TRAIL_N &&

            s->nal_unit_type != NAL_TSA_N   &&

            s->nal_unit_type != NAL_STSA_N  &&

            s->nal_unit_type != NAL_RADL_N  &&

            s->nal_unit_type != NAL_RADL_R  &&

            s->nal_unit_type != NAL_RASL_N  &&

            s->nal_unit_type != NAL_RASL_R)

            s->pocTid0 = s->poc;



        if (s->sps->sao_enabled) {

            sh->slice_sample_adaptive_offset_flag[0] = get_bits1(gb);

            if (s->sps->chroma_format_idc) {

                sh->slice_sample_adaptive_offset_flag[1] =

                sh->slice_sample_adaptive_offset_flag[2] = get_bits1(gb);

            }

        } else {

            sh->slice_sample_adaptive_offset_flag[0] = 0;

            sh->slice_sample_adaptive_offset_flag[1] = 0;

            sh->slice_sample_adaptive_offset_flag[2] = 0;

        }



        sh->nb_refs[L0] = sh->nb_refs[L1] = 0;

        if (sh->slice_type == P_SLICE || sh->slice_type == B_SLICE) {

            int nb_refs;



            sh->nb_refs[L0] = s->pps->num_ref_idx_l0_default_active;

            if (sh->slice_type == B_SLICE)

                sh->nb_refs[L1] = s->pps->num_ref_idx_l1_default_active;



            if (get_bits1(gb)) { // num_ref_idx_active_override_flag

                sh->nb_refs[L0] = get_ue_golomb_long(gb) + 1;

                if (sh->slice_type == B_SLICE)

                    sh->nb_refs[L1] = get_ue_golomb_long(gb) + 1;

            }

            if (sh->nb_refs[L0] > MAX_REFS || sh->nb_refs[L1] > MAX_REFS) {

                av_log(s->avctx, AV_LOG_ERROR, "Too many refs: %d/%d.\n",

                       sh->nb_refs[L0], sh->nb_refs[L1]);

                return AVERROR_INVALIDDATA;

            }



            sh->rpl_modification_flag[0] = 0;

            sh->rpl_modification_flag[1] = 0;

            nb_refs = ff_hevc_frame_nb_refs(s);

            if (!nb_refs) {

                av_log(s->avctx, AV_LOG_ERROR, "Zero refs for a frame with P or B slices.\n");

                return AVERROR_INVALIDDATA;

            }



            if (s->pps->lists_modification_present_flag && nb_refs > 1) {

                sh->rpl_modification_flag[0] = get_bits1(gb);

                if (sh->rpl_modification_flag[0]) {

                    for (i = 0; i < sh->nb_refs[L0]; i++)

                        sh->list_entry_lx[0][i] = get_bits(gb, av_ceil_log2(nb_refs));

                }



                if (sh->slice_type == B_SLICE) {

                    sh->rpl_modification_flag[1] = get_bits1(gb);

                    if (sh->rpl_modification_flag[1] == 1)

                        for (i = 0; i < sh->nb_refs[L1]; i++)

                            sh->list_entry_lx[1][i] = get_bits(gb, av_ceil_log2(nb_refs));

                }

            }



            if (sh->slice_type == B_SLICE)

                sh->mvd_l1_zero_flag = get_bits1(gb);



            if (s->pps->cabac_init_present_flag)

                sh->cabac_init_flag = get_bits1(gb);

            else

                sh->cabac_init_flag = 0;



            sh->collocated_ref_idx = 0;

            if (sh->slice_temporal_mvp_enabled_flag) {

                sh->collocated_list = L0;

                if (sh->slice_type == B_SLICE)

                    sh->collocated_list = !get_bits1(gb);



                if (sh->nb_refs[sh->collocated_list] > 1) {

                    sh->collocated_ref_idx = get_ue_golomb_long(gb);

                    if (sh->collocated_ref_idx >= sh->nb_refs[sh->collocated_list]) {

                        av_log(s->avctx, AV_LOG_ERROR,

                               "Invalid collocated_ref_idx: %d.\n",

                               sh->collocated_ref_idx);

                        return AVERROR_INVALIDDATA;

                    }

                }

            }



            if ((s->pps->weighted_pred_flag   && sh->slice_type == P_SLICE) ||

                (s->pps->weighted_bipred_flag && sh->slice_type == B_SLICE)) {

                pred_weight_table(s, gb);

            }



            sh->max_num_merge_cand = 5 - get_ue_golomb_long(gb);

            if (sh->max_num_merge_cand < 1 || sh->max_num_merge_cand > 5) {

                av_log(s->avctx, AV_LOG_ERROR,

                       "Invalid number of merging MVP candidates: %d.\n",

                       sh->max_num_merge_cand);

                return AVERROR_INVALIDDATA;

            }

        }



        sh->slice_qp_delta = get_se_golomb(gb);



        if (s->pps->pic_slice_level_chroma_qp_offsets_present_flag) {

            sh->slice_cb_qp_offset = get_se_golomb(gb);

            sh->slice_cr_qp_offset = get_se_golomb(gb);

        } else {

            sh->slice_cb_qp_offset = 0;

            sh->slice_cr_qp_offset = 0;

        }



        if (s->pps->chroma_qp_offset_list_enabled_flag)

            sh->cu_chroma_qp_offset_enabled_flag = get_bits1(gb);

        else

            sh->cu_chroma_qp_offset_enabled_flag = 0;



        if (s->pps->deblocking_filter_control_present_flag) {

            int deblocking_filter_override_flag = 0;



            if (s->pps->deblocking_filter_override_enabled_flag)

                deblocking_filter_override_flag = get_bits1(gb);



            if (deblocking_filter_override_flag) {

                sh->disable_deblocking_filter_flag = get_bits1(gb);

                if (!sh->disable_deblocking_filter_flag) {

                    sh->beta_offset = get_se_golomb(gb) * 2;

                    sh->tc_offset   = get_se_golomb(gb) * 2;

                }

            } else {

                sh->disable_deblocking_filter_flag = s->pps->disable_dbf;

                sh->beta_offset                    = s->pps->beta_offset;

                sh->tc_offset                      = s->pps->tc_offset;

            }

        } else {

            sh->disable_deblocking_filter_flag = 0;

            sh->beta_offset                    = 0;

            sh->tc_offset                      = 0;

        }



        if (s->pps->seq_loop_filter_across_slices_enabled_flag &&

            (sh->slice_sample_adaptive_offset_flag[0] ||

             sh->slice_sample_adaptive_offset_flag[1] ||

             !sh->disable_deblocking_filter_flag)) {

            sh->slice_loop_filter_across_slices_enabled_flag = get_bits1(gb);

        } else {

            sh->slice_loop_filter_across_slices_enabled_flag = s->pps->seq_loop_filter_across_slices_enabled_flag;

        }

    } else if (!s->slice_initialized) {

        av_log(s->avctx, AV_LOG_ERROR, "Independent slice segment missing.\n");

        return AVERROR_INVALIDDATA;

    }



    sh->num_entry_point_offsets = 0;

    if (s->pps->tiles_enabled_flag || s->pps->entropy_coding_sync_enabled_flag) {

        unsigned num_entry_point_offsets = get_ue_golomb_long(gb);

        // It would be possible to bound this tighter but this here is simpler

        if (sh->num_entry_point_offsets > get_bits_left(gb)) {

            av_log(s->avctx, AV_LOG_ERROR, "num_entry_point_offsets %d is invalid\n", num_entry_point_offsets);

            return AVERROR_INVALIDDATA;

        }



        sh->num_entry_point_offsets = num_entry_point_offsets;

        if (sh->num_entry_point_offsets > 0) {

            int offset_len = get_ue_golomb_long(gb) + 1;



            if (offset_len < 1 || offset_len > 32) {

                sh->num_entry_point_offsets = 0;

                av_log(s->avctx, AV_LOG_ERROR, "offset_len %d is invalid\n", offset_len);

                return AVERROR_INVALIDDATA;

            }



            av_freep(&sh->entry_point_offset);

            av_freep(&sh->offset);

            av_freep(&sh->size);

            sh->entry_point_offset = av_malloc_array(sh->num_entry_point_offsets, sizeof(int));

            sh->offset = av_malloc_array(sh->num_entry_point_offsets, sizeof(int));

            sh->size = av_malloc_array(sh->num_entry_point_offsets, sizeof(int));

            if (!sh->entry_point_offset || !sh->offset || !sh->size) {

                sh->num_entry_point_offsets = 0;

                av_log(s->avctx, AV_LOG_ERROR, "Failed to allocate memory\n");

                return AVERROR(ENOMEM);

            }

            for (i = 0; i < sh->num_entry_point_offsets; i++) {

                unsigned val = get_bits_long(gb, offset_len);

                sh->entry_point_offset[i] = val + 1; // +1; // +1 to get the size

            }

            if (s->threads_number > 1 && (s->pps->num_tile_rows > 1 || s->pps->num_tile_columns > 1)) {

                s->enable_parallel_tiles = 0; // TODO: you can enable tiles in parallel here

                s->threads_number = 1;

            } else

                s->enable_parallel_tiles = 0;

        } else

            s->enable_parallel_tiles = 0;

    }



    if (s->pps->slice_header_extension_present_flag) {

        unsigned int length = get_ue_golomb_long(gb);

        if (length*8LL > get_bits_left(gb)) {

            av_log(s->avctx, AV_LOG_ERROR, "too many slice_header_extension_data_bytes\n");

            return AVERROR_INVALIDDATA;

        }

        for (i = 0; i < length; i++)

            skip_bits(gb, 8);  // slice_header_extension_data_byte

    }



    // Inferred parameters

    sh->slice_qp = 26U + s->pps->pic_init_qp_minus26 + sh->slice_qp_delta;

    if (sh->slice_qp > 51 ||

        sh->slice_qp < -s->sps->qp_bd_offset) {

        av_log(s->avctx, AV_LOG_ERROR,

               "The slice_qp %d is outside the valid range "

               "[%d, 51].\n",

               sh->slice_qp,

               -s->sps->qp_bd_offset);

        return AVERROR_INVALIDDATA;

    }



    sh->slice_ctb_addr_rs = sh->slice_segment_addr;



    if (!s->sh.slice_ctb_addr_rs && s->sh.dependent_slice_segment_flag) {

        av_log(s->avctx, AV_LOG_ERROR, "Impossible slice segment.\n");

        return AVERROR_INVALIDDATA;

    }



    if (get_bits_left(gb) < 0) {

        av_log(s->avctx, AV_LOG_ERROR,

               "Overread slice header by %d bits\n", -get_bits_left(gb));

        return AVERROR_INVALIDDATA;

    }



    s->HEVClc->first_qp_group = !s->sh.dependent_slice_segment_flag;



    if (!s->pps->cu_qp_delta_enabled_flag)

        s->HEVClc->qp_y = s->sh.slice_qp;



    s->slice_initialized = 1;

    s->HEVClc->tu.cu_qp_offset_cb = 0;

    s->HEVClc->tu.cu_qp_offset_cr = 0;



    return 0;

}
