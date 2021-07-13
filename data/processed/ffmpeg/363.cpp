static void fill_slice_long(AVCodecContext *avctx, DXVA_Slice_H264_Long *slice,

                            unsigned position, unsigned size)

{

    const H264Context *h = avctx->priv_data;

    struct dxva_context *ctx = avctx->hwaccel_context;

    unsigned list;



    memset(slice, 0, sizeof(*slice));

    slice->BSNALunitDataLocation = position;

    slice->SliceBytesInBuffer    = size;

    slice->wBadSliceChopping     = 0;



    slice->first_mb_in_slice     = (h->mb_y >> FIELD_OR_MBAFF_PICTURE(h)) * h->mb_width + h->mb_x;

    slice->NumMbsForSlice        = 0; /* XXX it is set once we have all slices */

    slice->BitOffsetToSliceData  = get_bits_count(&h->gb);

    slice->slice_type            = ff_h264_get_slice_type(h);

    if (h->slice_type_fixed)

        slice->slice_type += 5;

    slice->luma_log2_weight_denom       = h->luma_log2_weight_denom;

    slice->chroma_log2_weight_denom     = h->chroma_log2_weight_denom;

    if (h->list_count > 0)

        slice->num_ref_idx_l0_active_minus1 = h->ref_count[0] - 1;

    if (h->list_count > 1)

        slice->num_ref_idx_l1_active_minus1 = h->ref_count[1] - 1;

    slice->slice_alpha_c0_offset_div2   = h->slice_alpha_c0_offset / 2;

    slice->slice_beta_offset_div2       = h->slice_beta_offset     / 2;

    slice->Reserved8Bits                = 0;



    for (list = 0; list < 2; list++) {

        unsigned i;

        for (i = 0; i < FF_ARRAY_ELEMS(slice->RefPicList[list]); i++) {

            if (list < h->list_count && i < h->ref_count[list]) {

                const Picture *r = &h->ref_list[list][i];

                unsigned plane;

                fill_picture_entry(&slice->RefPicList[list][i],

                                   ff_dxva2_get_surface_index(ctx, r),

                                   r->reference == PICT_BOTTOM_FIELD);

                for (plane = 0; plane < 3; plane++) {

                    int w, o;

                    if (plane == 0 && h->luma_weight_flag[list]) {

                        w = h->luma_weight[i][list][0];

                        o = h->luma_weight[i][list][1];

                    } else if (plane >= 1 && h->chroma_weight_flag[list]) {

                        w = h->chroma_weight[i][list][plane-1][0];

                        o = h->chroma_weight[i][list][plane-1][1];

                    } else {

                        w = 1 << (plane == 0 ? h->luma_log2_weight_denom :

                                               h->chroma_log2_weight_denom);

                        o = 0;

                    }

                    slice->Weights[list][i][plane][0] = w;

                    slice->Weights[list][i][plane][1] = o;

                }

            } else {

                unsigned plane;

                slice->RefPicList[list][i].bPicEntry = 0xff;

                for (plane = 0; plane < 3; plane++) {

                    slice->Weights[list][i][plane][0] = 0;

                    slice->Weights[list][i][plane][1] = 0;

                }

            }

        }

    }

    slice->slice_qs_delta    = 0; /* XXX not implemented by FFmpeg */

    slice->slice_qp_delta    = h->qscale - h->pps.init_qp;

    slice->redundant_pic_cnt = h->redundant_pic_count;

    if (h->slice_type == AV_PICTURE_TYPE_B)

        slice->direct_spatial_mv_pred_flag = h->direct_spatial_mv_pred;

    slice->cabac_init_idc = h->pps.cabac ? h->cabac_init_idc : 0;

    if (h->deblocking_filter < 2)

        slice->disable_deblocking_filter_idc = 1 - h->deblocking_filter;

    else

        slice->disable_deblocking_filter_idc = h->deblocking_filter;

    slice->slice_id = h->current_slice - 1;

}
