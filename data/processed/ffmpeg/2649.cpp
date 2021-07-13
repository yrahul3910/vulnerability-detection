int ff_hevc_decode_nal_sps(HEVCContext *s)

{

    const AVPixFmtDescriptor *desc;

    GetBitContext *gb = &s->HEVClc->gb;

    int ret    = 0;

    int sps_id = 0;

    int log2_diff_max_min_transform_block_size;

    int bit_depth_chroma, start, vui_present, sublayer_ordering_info;

    int i;



    HEVCSPS *sps;

    AVBufferRef *sps_buf = av_buffer_allocz(sizeof(*sps));



    if (!sps_buf)

        return AVERROR(ENOMEM);

    sps = (HEVCSPS*)sps_buf->data;



    av_log(s->avctx, AV_LOG_DEBUG, "Decoding SPS\n");



    // Coded parameters



    sps->vps_id = get_bits(gb, 4);

    if (sps->vps_id >= MAX_VPS_COUNT) {

        av_log(s->avctx, AV_LOG_ERROR, "VPS id out of range: %d\n", sps->vps_id);

        ret = AVERROR_INVALIDDATA;

        goto err;

    }



    if (!s->vps_list[sps->vps_id]) {

        av_log(s->avctx, AV_LOG_ERROR, "VPS does not exist \n");

        ret = AVERROR_INVALIDDATA;

        goto err;

    }



    sps->max_sub_layers = get_bits(gb, 3) + 1;

    if (sps->max_sub_layers > MAX_SUB_LAYERS) {

        av_log(s->avctx, AV_LOG_ERROR, "sps_max_sub_layers out of range: %d\n",

               sps->max_sub_layers);

        ret = AVERROR_INVALIDDATA;

        goto err;

    }



    skip_bits1(gb); // temporal_id_nesting_flag



    parse_ptl(s, &sps->ptl, sps->max_sub_layers);



    sps_id = get_ue_golomb_long(gb);

    if (sps_id >= MAX_SPS_COUNT) {

        av_log(s->avctx, AV_LOG_ERROR, "SPS id out of range: %d\n", sps_id);

        ret = AVERROR_INVALIDDATA;

        goto err;

    }



    sps->chroma_format_idc = get_ue_golomb_long(gb);

    if (sps->chroma_format_idc != 1) {

        avpriv_report_missing_feature(s->avctx, "chroma_format_idc != 1\n");

        ret = AVERROR_PATCHWELCOME;

        goto err;

    }



    if (sps->chroma_format_idc == 3)

        sps->separate_colour_plane_flag = get_bits1(gb);



    sps->width  = get_ue_golomb_long(gb);

    sps->height = get_ue_golomb_long(gb);

    if ((ret = av_image_check_size(sps->width,

                                   sps->height, 0, s->avctx)) < 0)

        goto err;



    if (get_bits1(gb)) { // pic_conformance_flag

        //TODO: * 2 is only valid for 420

        sps->pic_conf_win.left_offset   = get_ue_golomb_long(gb) * 2;

        sps->pic_conf_win.right_offset  = get_ue_golomb_long(gb) * 2;

        sps->pic_conf_win.top_offset    = get_ue_golomb_long(gb) * 2;

        sps->pic_conf_win.bottom_offset = get_ue_golomb_long(gb) * 2;



        if (s->avctx->flags2 & CODEC_FLAG2_IGNORE_CROP) {

            av_log(s->avctx, AV_LOG_DEBUG,

                   "discarding sps conformance window, "

                   "original values are l:%u r:%u t:%u b:%u\n",

                   sps->pic_conf_win.left_offset,

                   sps->pic_conf_win.right_offset,

                   sps->pic_conf_win.top_offset,

                   sps->pic_conf_win.bottom_offset);



            sps->pic_conf_win.left_offset   =

            sps->pic_conf_win.right_offset  =

            sps->pic_conf_win.top_offset    =

            sps->pic_conf_win.bottom_offset = 0;

        }

        sps->output_window = sps->pic_conf_win;

    }



    sps->bit_depth   = get_ue_golomb_long(gb) + 8;

    bit_depth_chroma = get_ue_golomb_long(gb) + 8;

    if (bit_depth_chroma != sps->bit_depth) {

        av_log(s->avctx, AV_LOG_ERROR,

               "Luma bit depth (%d) is different from chroma bit depth (%d), "

               "this is unsupported.\n",

               sps->bit_depth, bit_depth_chroma);

        ret = AVERROR_INVALIDDATA;

        goto err;

    }



    if (sps->chroma_format_idc == 1) {

        switch (sps->bit_depth) {

        case 8:  sps->pix_fmt = AV_PIX_FMT_YUV420P;   break;

        case 9:  sps->pix_fmt = AV_PIX_FMT_YUV420P9;  break;

        case 10: sps->pix_fmt = AV_PIX_FMT_YUV420P10; break;

        default:

            av_log(s->avctx, AV_LOG_ERROR, "Unsupported bit depth: %d\n",

                   sps->bit_depth);

            ret = AVERROR_PATCHWELCOME;

            goto err;

        }

    } else {

        av_log(s->avctx, AV_LOG_ERROR,

               "non-4:2:0 support is currently unspecified.\n");

        return AVERROR_PATCHWELCOME;

    }



    desc = av_pix_fmt_desc_get(sps->pix_fmt);

    if (!desc) {

        ret = AVERROR(EINVAL);

        goto err;

    }



    sps->hshift[0] = sps->vshift[0] = 0;

    sps->hshift[2] = sps->hshift[1] = desc->log2_chroma_w;

    sps->vshift[2] = sps->vshift[1] = desc->log2_chroma_h;



    sps->pixel_shift = sps->bit_depth > 8;



    sps->log2_max_poc_lsb = get_ue_golomb_long(gb) + 4;

    if (sps->log2_max_poc_lsb > 16) {

        av_log(s->avctx, AV_LOG_ERROR, "log2_max_pic_order_cnt_lsb_minus4 out range: %d\n",

               sps->log2_max_poc_lsb - 4);

        ret = AVERROR_INVALIDDATA;

        goto err;

    }



    sublayer_ordering_info = get_bits1(gb);

    start = sublayer_ordering_info ? 0 : sps->max_sub_layers - 1;

    for (i = start; i < sps->max_sub_layers; i++) {

        sps->temporal_layer[i].max_dec_pic_buffering = get_ue_golomb_long(gb) + 1;

        sps->temporal_layer[i].num_reorder_pics      = get_ue_golomb_long(gb);

        sps->temporal_layer[i].max_latency_increase  = get_ue_golomb_long(gb) - 1;

        if (sps->temporal_layer[i].max_dec_pic_buffering > MAX_DPB_SIZE) {

            av_log(s->avctx, AV_LOG_ERROR, "sps_max_dec_pic_buffering_minus1 out of range: %d\n",

                   sps->temporal_layer[i].max_dec_pic_buffering - 1);

            ret = AVERROR_INVALIDDATA;

            goto err;

        }

        if (sps->temporal_layer[i].num_reorder_pics > sps->temporal_layer[i].max_dec_pic_buffering - 1) {

            av_log(s->avctx, AV_LOG_ERROR, "sps_max_num_reorder_pics out of range: %d\n",

                   sps->temporal_layer[i].num_reorder_pics);

            ret = AVERROR_INVALIDDATA;

            goto err;

        }

    }



    if (!sublayer_ordering_info) {

        for (i = 0; i < start; i++) {

            sps->temporal_layer[i].max_dec_pic_buffering = sps->temporal_layer[start].max_dec_pic_buffering;

            sps->temporal_layer[i].num_reorder_pics      = sps->temporal_layer[start].num_reorder_pics;

            sps->temporal_layer[i].max_latency_increase  = sps->temporal_layer[start].max_latency_increase;

        }

    }



    sps->log2_min_cb_size                    = get_ue_golomb_long(gb) + 3;

    sps->log2_diff_max_min_coding_block_size = get_ue_golomb_long(gb);

    sps->log2_min_tb_size                    = get_ue_golomb_long(gb) + 2;

    log2_diff_max_min_transform_block_size   = get_ue_golomb_long(gb);

    sps->log2_max_trafo_size                 = log2_diff_max_min_transform_block_size +

                                               sps->log2_min_tb_size;



    if (sps->log2_min_tb_size >= sps->log2_min_cb_size) {

        av_log(s->avctx, AV_LOG_ERROR, "Invalid value for log2_min_tb_size");

        ret = AVERROR_INVALIDDATA;

        goto err;

    }

    sps->max_transform_hierarchy_depth_inter = get_ue_golomb_long(gb);

    sps->max_transform_hierarchy_depth_intra = get_ue_golomb_long(gb);



    sps->scaling_list_enable_flag = get_bits1(gb);

    if (sps->scaling_list_enable_flag) {

        set_default_scaling_list_data(&sps->scaling_list);



        if (get_bits1(gb)) {

            ret = scaling_list_data(s, &sps->scaling_list);

            if (ret < 0)

                goto err;

        }

    }



    sps->amp_enabled_flag = get_bits1(gb);

    sps->sao_enabled      = get_bits1(gb);



    sps->pcm_enabled_flag = get_bits1(gb);

    if (sps->pcm_enabled_flag) {

        sps->pcm.bit_depth   = get_bits(gb, 4) + 1;

        sps->pcm.bit_depth_chroma = get_bits(gb, 4) + 1;

        sps->pcm.log2_min_pcm_cb_size = get_ue_golomb_long(gb) + 3;

        sps->pcm.log2_max_pcm_cb_size = sps->pcm.log2_min_pcm_cb_size +

                                        get_ue_golomb_long(gb);

        if (sps->pcm.bit_depth > sps->bit_depth) {

            av_log(s->avctx, AV_LOG_ERROR,

                   "PCM bit depth (%d) is greater than normal bit depth (%d)\n",

                   sps->pcm.bit_depth, sps->bit_depth);

            ret = AVERROR_INVALIDDATA;

            goto err;

        }



        sps->pcm.loop_filter_disable_flag = get_bits1(gb);

    }



    sps->nb_st_rps = get_ue_golomb_long(gb);

    if (sps->nb_st_rps > MAX_SHORT_TERM_RPS_COUNT) {

        av_log(s->avctx, AV_LOG_ERROR, "Too many short term RPS: %d.\n",

               sps->nb_st_rps);

        ret = AVERROR_INVALIDDATA;

        goto err;

    }

    for (i = 0; i < sps->nb_st_rps; i++) {

        if ((ret = ff_hevc_decode_short_term_rps(s, &sps->st_rps[i],

                                                 sps, 0)) < 0)

            goto err;

    }



    sps->long_term_ref_pics_present_flag = get_bits1(gb);

    if (sps->long_term_ref_pics_present_flag) {

        sps->num_long_term_ref_pics_sps = get_ue_golomb_long(gb);

        for (i = 0; i < sps->num_long_term_ref_pics_sps; i++) {

            sps->lt_ref_pic_poc_lsb_sps[i]       = get_bits(gb, sps->log2_max_poc_lsb);

            sps->used_by_curr_pic_lt_sps_flag[i] = get_bits1(gb);

        }

    }



    sps->sps_temporal_mvp_enabled_flag          = get_bits1(gb);

    sps->sps_strong_intra_smoothing_enable_flag = get_bits1(gb);

    sps->vui.sar = (AVRational){0, 1};

    vui_present = get_bits1(gb);

    if (vui_present)

        decode_vui(s, sps);

    skip_bits1(gb); // sps_extension_flag



    if (s->apply_defdispwin) {

        sps->output_window.left_offset   += sps->vui.def_disp_win.left_offset;

        sps->output_window.right_offset  += sps->vui.def_disp_win.right_offset;

        sps->output_window.top_offset    += sps->vui.def_disp_win.top_offset;

        sps->output_window.bottom_offset += sps->vui.def_disp_win.bottom_offset;

    }

    if (sps->output_window.left_offset & (0x1F >> (sps->pixel_shift)) &&

        !(s->avctx->flags & CODEC_FLAG_UNALIGNED)) {

        sps->output_window.left_offset &= ~(0x1F >> (sps->pixel_shift));

        av_log(s->avctx, AV_LOG_WARNING, "Reducing left output window to %d "

               "chroma samples to preserve alignment.\n",

               sps->output_window.left_offset);

    }

    sps->output_width  = sps->width -

                         (sps->output_window.left_offset + sps->output_window.right_offset);

    sps->output_height = sps->height -

                         (sps->output_window.top_offset + sps->output_window.bottom_offset);

    if (sps->output_width <= 0 || sps->output_height <= 0) {

        av_log(s->avctx, AV_LOG_WARNING, "Invalid visible frame dimensions: %dx%d.\n",

               sps->output_width, sps->output_height);

        if (s->avctx->err_recognition & AV_EF_EXPLODE) {

            ret = AVERROR_INVALIDDATA;

            goto err;

        }

        av_log(s->avctx, AV_LOG_WARNING,

               "Displaying the whole video surface.\n");

        sps->pic_conf_win.left_offset   =

        sps->pic_conf_win.right_offset  =

        sps->pic_conf_win.top_offset    =

        sps->pic_conf_win.bottom_offset = 0;

        sps->output_width               = sps->width;

        sps->output_height              = sps->height;

    }



    // Inferred parameters

    sps->log2_ctb_size = sps->log2_min_cb_size +

                         sps->log2_diff_max_min_coding_block_size;

    sps->log2_min_pu_size = sps->log2_min_cb_size - 1;



    sps->ctb_width  = (sps->width  + (1 << sps->log2_ctb_size) - 1) >> sps->log2_ctb_size;

    sps->ctb_height = (sps->height + (1 << sps->log2_ctb_size) - 1) >> sps->log2_ctb_size;

    sps->ctb_size   = sps->ctb_width * sps->ctb_height;



    sps->min_cb_width  = sps->width  >> sps->log2_min_cb_size;

    sps->min_cb_height = sps->height >> sps->log2_min_cb_size;

    sps->min_tb_width  = sps->width  >> sps->log2_min_tb_size;

    sps->min_tb_height = sps->height >> sps->log2_min_tb_size;

    sps->min_pu_width  = sps->width  >> sps->log2_min_pu_size;

    sps->min_pu_height = sps->height >> sps->log2_min_pu_size;



    sps->qp_bd_offset = 6 * (sps->bit_depth - 8);



    if (sps->width  & ((1 << sps->log2_min_cb_size) - 1) ||

        sps->height & ((1 << sps->log2_min_cb_size) - 1)) {

        av_log(s->avctx, AV_LOG_ERROR, "Invalid coded frame dimensions.\n");

        goto err;

    }



    if (sps->log2_ctb_size > MAX_LOG2_CTB_SIZE) {

        av_log(s->avctx, AV_LOG_ERROR, "CTB size out of range: 2^%d\n", sps->log2_ctb_size);

        goto err;

    }

    if (sps->max_transform_hierarchy_depth_inter > sps->log2_ctb_size - sps->log2_min_tb_size) {

        av_log(s->avctx, AV_LOG_ERROR, "max_transform_hierarchy_depth_inter out of range: %d\n",

               sps->max_transform_hierarchy_depth_inter);

        goto err;

    }

    if (sps->max_transform_hierarchy_depth_intra > sps->log2_ctb_size - sps->log2_min_tb_size) {

        av_log(s->avctx, AV_LOG_ERROR, "max_transform_hierarchy_depth_intra out of range: %d\n",

               sps->max_transform_hierarchy_depth_intra);

        goto err;

    }

    if (sps->log2_max_trafo_size > FFMIN(sps->log2_ctb_size, 5)) {

        av_log(s->avctx, AV_LOG_ERROR,

               "max transform block size out of range: %d\n",

               sps->log2_max_trafo_size);

        goto err;

    }



    if (s->avctx->debug & FF_DEBUG_BITSTREAM) {

        av_log(s->avctx, AV_LOG_DEBUG,

               "Parsed SPS: id %d; coded wxh: %dx%d; "

               "cropped wxh: %dx%d; pix_fmt: %s.\n",

               sps_id, sps->width, sps->height,

               sps->output_width, sps->output_height,

               av_get_pix_fmt_name(sps->pix_fmt));

    }



    /* check if this is a repeat of an already parsed SPS, then keep the

     * original one.

     * otherwise drop all PPSes that depend on it */

    if (s->sps_list[sps_id] &&

        !memcmp(s->sps_list[sps_id]->data, sps_buf->data, sps_buf->size)) {

        av_buffer_unref(&sps_buf);

    } else {

        for (i = 0; i < FF_ARRAY_ELEMS(s->pps_list); i++) {

            if (s->pps_list[i] && ((HEVCPPS*)s->pps_list[i]->data)->sps_id == sps_id)

                av_buffer_unref(&s->pps_list[i]);

        }

        av_buffer_unref(&s->sps_list[sps_id]);

        s->sps_list[sps_id] = sps_buf;

    }



    return 0;



err:

    av_buffer_unref(&sps_buf);

    return ret;

}
