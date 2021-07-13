int ff_hevc_decode_nal_pps(HEVCContext *s)

{

    GetBitContext *gb = &s->HEVClc->gb;

    HEVCSPS      *sps = NULL;

    int pic_area_in_ctbs;

    int log2_diff_ctb_min_tb_size;

    int i, j, x, y, ctb_addr_rs, tile_id;

    int ret = 0;

    unsigned int pps_id = 0;



    AVBufferRef *pps_buf;

    HEVCPPS *pps = av_mallocz(sizeof(*pps));



    if (!pps)

        return AVERROR(ENOMEM);



    pps_buf = av_buffer_create((uint8_t *)pps, sizeof(*pps),

                               hevc_pps_free, NULL, 0);

    if (!pps_buf) {

        av_freep(&pps);

        return AVERROR(ENOMEM);

    }



    av_log(s->avctx, AV_LOG_DEBUG, "Decoding PPS\n");



    // Default values

    pps->loop_filter_across_tiles_enabled_flag = 1;

    pps->num_tile_columns                      = 1;

    pps->num_tile_rows                         = 1;

    pps->uniform_spacing_flag                  = 1;

    pps->disable_dbf                           = 0;

    pps->beta_offset                           = 0;

    pps->tc_offset                             = 0;

    pps->log2_max_transform_skip_block_size    = 2;



    // Coded parameters

    pps_id = get_ue_golomb_long(gb);

    if (pps_id >= MAX_PPS_COUNT) {

        av_log(s->avctx, AV_LOG_ERROR, "PPS id out of range: %d\n", pps_id);

        ret = AVERROR_INVALIDDATA;

        goto err;

    }

    pps->sps_id = get_ue_golomb_long(gb);

    if (pps->sps_id >= MAX_SPS_COUNT) {

        av_log(s->avctx, AV_LOG_ERROR, "SPS id out of range: %d\n", pps->sps_id);

        ret = AVERROR_INVALIDDATA;

        goto err;

    }

    if (!s->sps_list[pps->sps_id]) {

        av_log(s->avctx, AV_LOG_ERROR, "SPS %u does not exist.\n", pps->sps_id);

        ret = AVERROR_INVALIDDATA;

        goto err;

    }

    sps = (HEVCSPS *)s->sps_list[pps->sps_id]->data;



    pps->dependent_slice_segments_enabled_flag = get_bits1(gb);

    pps->output_flag_present_flag              = get_bits1(gb);

    pps->num_extra_slice_header_bits           = get_bits(gb, 3);



    pps->sign_data_hiding_flag = get_bits1(gb);



    pps->cabac_init_present_flag = get_bits1(gb);



    pps->num_ref_idx_l0_default_active = get_ue_golomb_long(gb) + 1;

    pps->num_ref_idx_l1_default_active = get_ue_golomb_long(gb) + 1;



    pps->pic_init_qp_minus26 = get_se_golomb(gb);



    pps->constrained_intra_pred_flag = get_bits1(gb);

    pps->transform_skip_enabled_flag = get_bits1(gb);



    pps->cu_qp_delta_enabled_flag = get_bits1(gb);

    pps->diff_cu_qp_delta_depth   = 0;

    if (pps->cu_qp_delta_enabled_flag)

        pps->diff_cu_qp_delta_depth = get_ue_golomb_long(gb);



    pps->cb_qp_offset = get_se_golomb(gb);

    if (pps->cb_qp_offset < -12 || pps->cb_qp_offset > 12) {

        av_log(s->avctx, AV_LOG_ERROR, "pps_cb_qp_offset out of range: %d\n",

               pps->cb_qp_offset);

        ret = AVERROR_INVALIDDATA;

        goto err;

    }

    pps->cr_qp_offset = get_se_golomb(gb);

    if (pps->cr_qp_offset < -12 || pps->cr_qp_offset > 12) {

        av_log(s->avctx, AV_LOG_ERROR, "pps_cr_qp_offset out of range: %d\n",

               pps->cr_qp_offset);

        ret = AVERROR_INVALIDDATA;

        goto err;

    }

    pps->pic_slice_level_chroma_qp_offsets_present_flag = get_bits1(gb);



    pps->weighted_pred_flag   = get_bits1(gb);

    pps->weighted_bipred_flag = get_bits1(gb);



    pps->transquant_bypass_enable_flag    = get_bits1(gb);

    pps->tiles_enabled_flag               = get_bits1(gb);

    pps->entropy_coding_sync_enabled_flag = get_bits1(gb);



    if (pps->tiles_enabled_flag) {

        pps->num_tile_columns = get_ue_golomb_long(gb) + 1;

        pps->num_tile_rows    = get_ue_golomb_long(gb) + 1;

        if (pps->num_tile_columns == 0 ||

            pps->num_tile_columns >= sps->width) {

            av_log(s->avctx, AV_LOG_ERROR, "num_tile_columns_minus1 out of range: %d\n",

                   pps->num_tile_columns - 1);

            ret = AVERROR_INVALIDDATA;

            goto err;

        }

        if (pps->num_tile_rows == 0 ||

            pps->num_tile_rows >= sps->height) {

            av_log(s->avctx, AV_LOG_ERROR, "num_tile_rows_minus1 out of range: %d\n",

                   pps->num_tile_rows - 1);

            ret = AVERROR_INVALIDDATA;

            goto err;

        }



        pps->column_width = av_malloc_array(pps->num_tile_columns, sizeof(*pps->column_width));

        pps->row_height   = av_malloc_array(pps->num_tile_rows,    sizeof(*pps->row_height));

        if (!pps->column_width || !pps->row_height) {

            ret = AVERROR(ENOMEM);

            goto err;

        }



        pps->uniform_spacing_flag = get_bits1(gb);

        if (!pps->uniform_spacing_flag) {

            uint64_t sum = 0;

            for (i = 0; i < pps->num_tile_columns - 1; i++) {

                pps->column_width[i] = get_ue_golomb_long(gb) + 1;

                sum                 += pps->column_width[i];

            }

            if (sum >= sps->ctb_width) {

                av_log(s->avctx, AV_LOG_ERROR, "Invalid tile widths.\n");

                ret = AVERROR_INVALIDDATA;

                goto err;

            }

            pps->column_width[pps->num_tile_columns - 1] = sps->ctb_width - sum;



            sum = 0;

            for (i = 0; i < pps->num_tile_rows - 1; i++) {

                pps->row_height[i] = get_ue_golomb_long(gb) + 1;

                sum               += pps->row_height[i];

            }

            if (sum >= sps->ctb_height) {

                av_log(s->avctx, AV_LOG_ERROR, "Invalid tile heights.\n");

                ret = AVERROR_INVALIDDATA;

                goto err;

            }

            pps->row_height[pps->num_tile_rows - 1] = sps->ctb_height - sum;

        }

        pps->loop_filter_across_tiles_enabled_flag = get_bits1(gb);

    }



    pps->seq_loop_filter_across_slices_enabled_flag = get_bits1(gb);



    pps->deblocking_filter_control_present_flag = get_bits1(gb);

    if (pps->deblocking_filter_control_present_flag) {

        pps->deblocking_filter_override_enabled_flag = get_bits1(gb);

        pps->disable_dbf                             = get_bits1(gb);

        if (!pps->disable_dbf) {

            pps->beta_offset = get_se_golomb(gb) * 2;

            pps->tc_offset = get_se_golomb(gb) * 2;

            if (pps->beta_offset/2 < -6 || pps->beta_offset/2 > 6) {

                av_log(s->avctx, AV_LOG_ERROR, "pps_beta_offset_div2 out of range: %d\n",

                       pps->beta_offset/2);

                ret = AVERROR_INVALIDDATA;

                goto err;

            }

            if (pps->tc_offset/2 < -6 || pps->tc_offset/2 > 6) {

                av_log(s->avctx, AV_LOG_ERROR, "pps_tc_offset_div2 out of range: %d\n",

                       pps->tc_offset/2);

                ret = AVERROR_INVALIDDATA;

                goto err;

            }

        }

    }



    pps->scaling_list_data_present_flag = get_bits1(gb);

    if (pps->scaling_list_data_present_flag) {

        set_default_scaling_list_data(&pps->scaling_list);

        ret = scaling_list_data(s, &pps->scaling_list, sps);

        if (ret < 0)

            goto err;

    }

    pps->lists_modification_present_flag = get_bits1(gb);

    pps->log2_parallel_merge_level       = get_ue_golomb_long(gb) + 2;

    if (pps->log2_parallel_merge_level > sps->log2_ctb_size) {

        av_log(s->avctx, AV_LOG_ERROR, "log2_parallel_merge_level_minus2 out of range: %d\n",

               pps->log2_parallel_merge_level - 2);

        ret = AVERROR_INVALIDDATA;

        goto err;

    }



    pps->slice_header_extension_present_flag = get_bits1(gb);



    if (get_bits1(gb)) { // pps_extension_present_flag

        int pps_range_extensions_flag = get_bits1(gb);

        /* int pps_extension_7bits = */ get_bits(gb, 7);

        if (sps->ptl.general_ptl.profile_idc == FF_PROFILE_HEVC_REXT && pps_range_extensions_flag) {

            pps_range_extensions(s, pps, sps);

        }

    }



    // Inferred parameters

    pps->col_bd   = av_malloc_array(pps->num_tile_columns + 1, sizeof(*pps->col_bd));

    pps->row_bd   = av_malloc_array(pps->num_tile_rows + 1,    sizeof(*pps->row_bd));

    pps->col_idxX = av_malloc_array(sps->ctb_width,    sizeof(*pps->col_idxX));

    if (!pps->col_bd || !pps->row_bd || !pps->col_idxX) {

        ret = AVERROR(ENOMEM);

        goto err;

    }



    if (pps->uniform_spacing_flag) {

        if (!pps->column_width) {

            pps->column_width = av_malloc_array(pps->num_tile_columns, sizeof(*pps->column_width));

            pps->row_height   = av_malloc_array(pps->num_tile_rows,    sizeof(*pps->row_height));

        }

        if (!pps->column_width || !pps->row_height) {

            ret = AVERROR(ENOMEM);

            goto err;

        }



        for (i = 0; i < pps->num_tile_columns; i++) {

            pps->column_width[i] = ((i + 1) * sps->ctb_width) / pps->num_tile_columns -

                                   (i * sps->ctb_width) / pps->num_tile_columns;

        }



        for (i = 0; i < pps->num_tile_rows; i++) {

            pps->row_height[i] = ((i + 1) * sps->ctb_height) / pps->num_tile_rows -

                                 (i * sps->ctb_height) / pps->num_tile_rows;

        }

    }



    pps->col_bd[0] = 0;

    for (i = 0; i < pps->num_tile_columns; i++)

        pps->col_bd[i + 1] = pps->col_bd[i] + pps->column_width[i];



    pps->row_bd[0] = 0;

    for (i = 0; i < pps->num_tile_rows; i++)

        pps->row_bd[i + 1] = pps->row_bd[i] + pps->row_height[i];



    for (i = 0, j = 0; i < sps->ctb_width; i++) {

        if (i > pps->col_bd[j])

            j++;

        pps->col_idxX[i] = j;

    }



    /**

     * 6.5

     */

    pic_area_in_ctbs     = sps->ctb_width    * sps->ctb_height;



    pps->ctb_addr_rs_to_ts = av_malloc_array(pic_area_in_ctbs,    sizeof(*pps->ctb_addr_rs_to_ts));

    pps->ctb_addr_ts_to_rs = av_malloc_array(pic_area_in_ctbs,    sizeof(*pps->ctb_addr_ts_to_rs));

    pps->tile_id           = av_malloc_array(pic_area_in_ctbs,    sizeof(*pps->tile_id));

    pps->min_tb_addr_zs_tab = av_malloc_array((sps->tb_mask+2) * (sps->tb_mask+2), sizeof(*pps->min_tb_addr_zs_tab));

    if (!pps->ctb_addr_rs_to_ts || !pps->ctb_addr_ts_to_rs ||

        !pps->tile_id || !pps->min_tb_addr_zs_tab) {

        ret = AVERROR(ENOMEM);

        goto err;

    }



    for (ctb_addr_rs = 0; ctb_addr_rs < pic_area_in_ctbs; ctb_addr_rs++) {

        int tb_x   = ctb_addr_rs % sps->ctb_width;

        int tb_y   = ctb_addr_rs / sps->ctb_width;

        int tile_x = 0;

        int tile_y = 0;

        int val    = 0;



        for (i = 0; i < pps->num_tile_columns; i++) {

            if (tb_x < pps->col_bd[i + 1]) {

                tile_x = i;

                break;

            }

        }



        for (i = 0; i < pps->num_tile_rows; i++) {

            if (tb_y < pps->row_bd[i + 1]) {

                tile_y = i;

                break;

            }

        }



        for (i = 0; i < tile_x; i++)

            val += pps->row_height[tile_y] * pps->column_width[i];

        for (i = 0; i < tile_y; i++)

            val += sps->ctb_width * pps->row_height[i];



        val += (tb_y - pps->row_bd[tile_y]) * pps->column_width[tile_x] +

               tb_x - pps->col_bd[tile_x];



        pps->ctb_addr_rs_to_ts[ctb_addr_rs] = val;

        pps->ctb_addr_ts_to_rs[val]         = ctb_addr_rs;

    }



    for (j = 0, tile_id = 0; j < pps->num_tile_rows; j++)

        for (i = 0; i < pps->num_tile_columns; i++, tile_id++)

            for (y = pps->row_bd[j]; y < pps->row_bd[j + 1]; y++)

                for (x = pps->col_bd[i]; x < pps->col_bd[i + 1]; x++)

                    pps->tile_id[pps->ctb_addr_rs_to_ts[y * sps->ctb_width + x]] = tile_id;



    pps->tile_pos_rs = av_malloc_array(tile_id, sizeof(*pps->tile_pos_rs));

    if (!pps->tile_pos_rs) {

        ret = AVERROR(ENOMEM);

        goto err;

    }



    for (j = 0; j < pps->num_tile_rows; j++)

        for (i = 0; i < pps->num_tile_columns; i++)

            pps->tile_pos_rs[j * pps->num_tile_columns + i] = pps->row_bd[j] * sps->ctb_width + pps->col_bd[i];



    log2_diff_ctb_min_tb_size = sps->log2_ctb_size - sps->log2_min_tb_size;

    pps->min_tb_addr_zs = &pps->min_tb_addr_zs_tab[1*(sps->tb_mask+2)+1];

    for (y = 0; y < sps->tb_mask+2; y++) {

        pps->min_tb_addr_zs_tab[y*(sps->tb_mask+2)] = -1;

        pps->min_tb_addr_zs_tab[y]    = -1;

    }

    for (y = 0; y < sps->tb_mask+1; y++) {

        for (x = 0; x < sps->tb_mask+1; x++) {

            int tb_x        = x >> log2_diff_ctb_min_tb_size;

            int tb_y        = y >> log2_diff_ctb_min_tb_size;

            int ctb_addr_rs = sps->ctb_width * tb_y + tb_x;

            int val         = pps->ctb_addr_rs_to_ts[ctb_addr_rs] <<

                              (log2_diff_ctb_min_tb_size * 2);

            for (i = 0; i < log2_diff_ctb_min_tb_size; i++) {

                int m = 1 << i;

                val += (m & x ? m * m : 0) + (m & y ? 2 * m * m : 0);

            }

            pps->min_tb_addr_zs[y * (sps->tb_mask+2) + x] = val;

        }

    }



    if (get_bits_left(gb) < 0) {

        av_log(s->avctx, AV_LOG_ERROR,

               "Overread PPS by %d bits\n", -get_bits_left(gb));

        goto err;

    }



    av_buffer_unref(&s->pps_list[pps_id]);

    s->pps_list[pps_id] = pps_buf;



    return 0;



err:

    av_buffer_unref(&pps_buf);

    return ret;

}
