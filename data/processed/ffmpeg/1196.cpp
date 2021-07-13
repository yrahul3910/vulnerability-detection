static int h264_slice_header_parse(const H264Context *h, H264SliceContext *sl,

                                   const H2645NAL *nal)

{

    const SPS *sps;

    const PPS *pps;

    int ret;

    unsigned int slice_type, tmp, i;

    int field_pic_flag, bottom_field_flag;

    int droppable, picture_structure;



    sl->first_mb_addr = get_ue_golomb(&sl->gb);



    slice_type = get_ue_golomb_31(&sl->gb);

    if (slice_type > 9) {

        av_log(h->avctx, AV_LOG_ERROR,

               "slice type %d too large at %d\n",

               slice_type, sl->first_mb_addr);

        return AVERROR_INVALIDDATA;

    }

    if (slice_type > 4) {

        slice_type -= 5;

        sl->slice_type_fixed = 1;

    } else

        sl->slice_type_fixed = 0;



    slice_type         = ff_h264_golomb_to_pict_type[slice_type];

    sl->slice_type     = slice_type;

    sl->slice_type_nos = slice_type & 3;



    if (nal->type  == NAL_IDR_SLICE &&

        sl->slice_type_nos != AV_PICTURE_TYPE_I) {

        av_log(h->avctx, AV_LOG_ERROR, "A non-intra slice in an IDR NAL unit.\n");

        return AVERROR_INVALIDDATA;

    }



    sl->pps_id = get_ue_golomb(&sl->gb);

    if (sl->pps_id >= MAX_PPS_COUNT) {

        av_log(h->avctx, AV_LOG_ERROR, "pps_id %u out of range\n", sl->pps_id);

        return AVERROR_INVALIDDATA;

    }

    if (!h->ps.pps_list[sl->pps_id]) {

        av_log(h->avctx, AV_LOG_ERROR,

               "non-existing PPS %u referenced\n",

               sl->pps_id);

        return AVERROR_INVALIDDATA;

    }

    pps = (const PPS*)h->ps.pps_list[sl->pps_id]->data;



    if (!h->ps.sps_list[pps->sps_id]) {

        av_log(h->avctx, AV_LOG_ERROR,

               "non-existing SPS %u referenced\n", pps->sps_id);

        return AVERROR_INVALIDDATA;

    }

    sps = (const SPS*)h->ps.sps_list[pps->sps_id]->data;



    sl->frame_num = get_bits(&sl->gb, sps->log2_max_frame_num);



    sl->mb_mbaff       = 0;



    droppable = nal->ref_idc == 0;

    if (sps->frame_mbs_only_flag) {

        picture_structure = PICT_FRAME;

    } else {

        field_pic_flag = get_bits1(&sl->gb);

        if (field_pic_flag) {

            bottom_field_flag = get_bits1(&sl->gb);

            picture_structure = PICT_TOP_FIELD + bottom_field_flag;

        } else {

            picture_structure = PICT_FRAME;

        }

    }

    sl->picture_structure      = picture_structure;

    sl->mb_field_decoding_flag = picture_structure != PICT_FRAME;



    if (picture_structure == PICT_FRAME) {

        sl->curr_pic_num = sl->frame_num;

        sl->max_pic_num  = 1 << sps->log2_max_frame_num;

    } else {

        sl->curr_pic_num = 2 * sl->frame_num + 1;

        sl->max_pic_num  = 1 << (sps->log2_max_frame_num + 1);

    }



    if (nal->type == NAL_IDR_SLICE)

        get_ue_golomb(&sl->gb); /* idr_pic_id */



    if (sps->poc_type == 0) {

        sl->poc_lsb = get_bits(&sl->gb, sps->log2_max_poc_lsb);



        if (pps->pic_order_present == 1 && picture_structure == PICT_FRAME)

            sl->delta_poc_bottom = get_se_golomb(&sl->gb);

    }



    if (sps->poc_type == 1 && !sps->delta_pic_order_always_zero_flag) {

        sl->delta_poc[0] = get_se_golomb(&sl->gb);



        if (pps->pic_order_present == 1 && picture_structure == PICT_FRAME)

            sl->delta_poc[1] = get_se_golomb(&sl->gb);

    }



    if (pps->redundant_pic_cnt_present)

        sl->redundant_pic_count = get_ue_golomb(&sl->gb);



    if (sl->slice_type_nos == AV_PICTURE_TYPE_B)

        sl->direct_spatial_mv_pred = get_bits1(&sl->gb);



    ret = ff_h264_parse_ref_count(&sl->list_count, sl->ref_count,

                                  &sl->gb, pps, sl->slice_type_nos,

                                  picture_structure);

    if (ret < 0)

        return ret;



    if (sl->slice_type_nos != AV_PICTURE_TYPE_I) {

       ret = ff_h264_decode_ref_pic_list_reordering(h, sl);

       if (ret < 0) {

           sl->ref_count[1] = sl->ref_count[0] = 0;

           return ret;

       }

    }



    sl->pwt.use_weight = 0;

    for (i = 0; i < 2; i++) {

        sl->pwt.luma_weight_flag[i]   = 0;

        sl->pwt.chroma_weight_flag[i] = 0;

    }

    if ((pps->weighted_pred && sl->slice_type_nos == AV_PICTURE_TYPE_P) ||

        (pps->weighted_bipred_idc == 1 &&

         sl->slice_type_nos == AV_PICTURE_TYPE_B))

        ff_h264_pred_weight_table(&sl->gb, sps, sl->ref_count,

                                  sl->slice_type_nos, &sl->pwt);



    sl->explicit_ref_marking = 0;

    if (nal->ref_idc) {

        ret = ff_h264_decode_ref_pic_marking(h, sl, &sl->gb);

        if (ret < 0 && (h->avctx->err_recognition & AV_EF_EXPLODE))

            return AVERROR_INVALIDDATA;

    }



    if (sl->slice_type_nos != AV_PICTURE_TYPE_I && pps->cabac) {

        tmp = get_ue_golomb_31(&sl->gb);

        if (tmp > 2) {

            av_log(h->avctx, AV_LOG_ERROR, "cabac_init_idc %u overflow\n", tmp);

            return AVERROR_INVALIDDATA;

        }

        sl->cabac_init_idc = tmp;

    }



    sl->last_qscale_diff = 0;

    tmp = pps->init_qp + get_se_golomb(&sl->gb);

    if (tmp > 51 + 6 * (sps->bit_depth_luma - 8)) {

        av_log(h->avctx, AV_LOG_ERROR, "QP %u out of range\n", tmp);

        return AVERROR_INVALIDDATA;

    }

    sl->qscale       = tmp;

    sl->chroma_qp[0] = get_chroma_qp(pps, 0, sl->qscale);

    sl->chroma_qp[1] = get_chroma_qp(pps, 1, sl->qscale);

    // FIXME qscale / qp ... stuff

    if (sl->slice_type == AV_PICTURE_TYPE_SP)

        get_bits1(&sl->gb); /* sp_for_switch_flag */

    if (sl->slice_type == AV_PICTURE_TYPE_SP ||

        sl->slice_type == AV_PICTURE_TYPE_SI)

        get_se_golomb(&sl->gb); /* slice_qs_delta */



    sl->deblocking_filter     = 1;

    sl->slice_alpha_c0_offset = 0;

    sl->slice_beta_offset     = 0;

    if (pps->deblocking_filter_parameters_present) {

        tmp = get_ue_golomb_31(&sl->gb);

        if (tmp > 2) {

            av_log(h->avctx, AV_LOG_ERROR,

                   "deblocking_filter_idc %u out of range\n", tmp);

            return AVERROR_INVALIDDATA;

        }

        sl->deblocking_filter = tmp;

        if (sl->deblocking_filter < 2)

            sl->deblocking_filter ^= 1;  // 1<->0



        if (sl->deblocking_filter) {

            sl->slice_alpha_c0_offset = get_se_golomb(&sl->gb) * 2;

            sl->slice_beta_offset     = get_se_golomb(&sl->gb) * 2;

            if (sl->slice_alpha_c0_offset >  12 ||

                sl->slice_alpha_c0_offset < -12 ||

                sl->slice_beta_offset >  12     ||

                sl->slice_beta_offset < -12) {

                av_log(h->avctx, AV_LOG_ERROR,

                       "deblocking filter parameters %d %d out of range\n",

                       sl->slice_alpha_c0_offset, sl->slice_beta_offset);

                return AVERROR_INVALIDDATA;

            }

        }

    }



    return 0;

}
