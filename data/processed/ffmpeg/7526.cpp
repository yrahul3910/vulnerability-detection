static int scaling_list_data(HEVCContext *s, ScalingList *sl)

{

    GetBitContext *gb = &s->HEVClc.gb;

    uint8_t scaling_list_pred_mode_flag[4][6];

    int32_t scaling_list_dc_coef[2][6];

    int size_id, matrix_id, i, pos, delta;



    for (size_id = 0; size_id < 4; size_id++)

        for (matrix_id = 0; matrix_id < (size_id == 3 ? 2 : 6); matrix_id++) {

            scaling_list_pred_mode_flag[size_id][matrix_id] = get_bits1(gb);

            if (!scaling_list_pred_mode_flag[size_id][matrix_id]) {

                delta = get_ue_golomb_long(gb);

                /* Only need to handle non-zero delta. Zero means default,

                 * which should already be in the arrays. */

                if (delta) {

                    // Copy from previous array.

                    if (matrix_id - delta < 0) {

                        av_log(s->avctx, AV_LOG_ERROR,

                               "Invalid delta in scaling list data: %d.\n", delta);

                        return AVERROR_INVALIDDATA;

                    }



                    memcpy(sl->sl[size_id][matrix_id],

                           sl->sl[size_id][matrix_id - delta],

                           size_id > 0 ? 64 : 16);

                    if (size_id > 1)

                        sl->sl_dc[size_id - 2][matrix_id] = sl->sl_dc[size_id - 2][matrix_id - delta];

                }

            } else {

                int next_coef, coef_num;

                int32_t scaling_list_delta_coef;



                next_coef = 8;

                coef_num  = FFMIN(64, 1 << (4 + (size_id << 1)));

                if (size_id > 1) {

                    scaling_list_dc_coef[size_id - 2][matrix_id] = get_se_golomb(gb) + 8;

                    next_coef = scaling_list_dc_coef[size_id - 2][matrix_id];

                    sl->sl_dc[size_id - 2][matrix_id] = next_coef;

                }

                for (i = 0; i < coef_num; i++) {

                    if (size_id == 0)

                        pos = 4 * ff_hevc_diag_scan4x4_y[i] +

                                  ff_hevc_diag_scan4x4_x[i];

                    else

                        pos = 8 * ff_hevc_diag_scan8x8_y[i] +

                                  ff_hevc_diag_scan8x8_x[i];



                    scaling_list_delta_coef = get_se_golomb(gb);

                    next_coef = (next_coef + scaling_list_delta_coef + 256) % 256;

                    sl->sl[size_id][matrix_id][pos] = next_coef;

                }

            }

        }



    return 0;

}
