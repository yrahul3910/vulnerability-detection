static inline int parse_nal_units(AVCodecParserContext *s, const uint8_t *buf,

                           int buf_size, AVCodecContext *avctx)

{

    HEVCParserContext *ctx = s->priv_data;

    HEVCContext       *h   = &ctx->h;

    GetBitContext      *gb;

    SliceHeader        *sh = &h->sh;

    HEVCParamSets *ps = &h->ps;

    HEVCSEIContext *sei = &h->sei;

    int is_global = buf == avctx->extradata;

    int i, ret;



    if (!h->HEVClc)

        h->HEVClc = av_mallocz(sizeof(HEVCLocalContext));

    if (!h->HEVClc)

        return AVERROR(ENOMEM);



    gb = &h->HEVClc->gb;



    /* set some sane default values */

    s->pict_type         = AV_PICTURE_TYPE_I;

    s->key_frame         = 0;

    s->picture_structure = AV_PICTURE_STRUCTURE_UNKNOWN;



    h->avctx = avctx;



    ff_hevc_reset_sei(sei);



    ret = ff_h2645_packet_split(&ctx->pkt, buf, buf_size, avctx, 0, 0,

                                AV_CODEC_ID_HEVC, 1);

    if (ret < 0)

        return ret;



    for (i = 0; i < ctx->pkt.nb_nals; i++) {

        H2645NAL *nal = &ctx->pkt.nals[i];

        int num = 0, den = 0;



        h->nal_unit_type = nal->type;

        h->temporal_id   = nal->temporal_id;

        *gb = nal->gb;



        switch (h->nal_unit_type) {

        case HEVC_NAL_VPS:

            ff_hevc_decode_nal_vps(gb, avctx, ps);

            break;

        case HEVC_NAL_SPS:

            ff_hevc_decode_nal_sps(gb, avctx, ps, 1);

            break;

        case HEVC_NAL_PPS:

            ff_hevc_decode_nal_pps(gb, avctx, ps);

            break;

        case HEVC_NAL_SEI_PREFIX:

        case HEVC_NAL_SEI_SUFFIX:

            ff_hevc_decode_nal_sei(gb, avctx, sei, ps, h->nal_unit_type);

            break;

        case HEVC_NAL_TRAIL_N:

        case HEVC_NAL_TRAIL_R:

        case HEVC_NAL_TSA_N:

        case HEVC_NAL_TSA_R:

        case HEVC_NAL_STSA_N:

        case HEVC_NAL_STSA_R:

        case HEVC_NAL_RADL_N:

        case HEVC_NAL_RADL_R:

        case HEVC_NAL_RASL_N:

        case HEVC_NAL_RASL_R:

        case HEVC_NAL_BLA_W_LP:

        case HEVC_NAL_BLA_W_RADL:

        case HEVC_NAL_BLA_N_LP:

        case HEVC_NAL_IDR_W_RADL:

        case HEVC_NAL_IDR_N_LP:

        case HEVC_NAL_CRA_NUT:



            if (is_global) {

                av_log(avctx, AV_LOG_ERROR, "Invalid NAL unit: %d\n", h->nal_unit_type);

                return AVERROR_INVALIDDATA;

            }



            sh->first_slice_in_pic_flag = get_bits1(gb);

            s->picture_structure = h->sei.picture_timing.picture_struct;

            s->field_order = h->sei.picture_timing.picture_struct;



            if (IS_IRAP(h)) {

                s->key_frame = 1;

                sh->no_output_of_prior_pics_flag = get_bits1(gb);

            }



            sh->pps_id = get_ue_golomb(gb);

            if (sh->pps_id >= HEVC_MAX_PPS_COUNT || !ps->pps_list[sh->pps_id]) {

                av_log(avctx, AV_LOG_ERROR, "PPS id out of range: %d\n", sh->pps_id);

                return AVERROR_INVALIDDATA;

            }

            ps->pps = (HEVCPPS*)ps->pps_list[sh->pps_id]->data;



            if (ps->pps->sps_id >= HEVC_MAX_SPS_COUNT || !ps->sps_list[ps->pps->sps_id]) {

                av_log(avctx, AV_LOG_ERROR, "SPS id out of range: %d\n", ps->pps->sps_id);

                return AVERROR_INVALIDDATA;

            }

            if (ps->sps != (HEVCSPS*)ps->sps_list[ps->pps->sps_id]->data) {

                ps->sps = (HEVCSPS*)ps->sps_list[ps->pps->sps_id]->data;

                ps->vps = (HEVCVPS*)ps->vps_list[ps->sps->vps_id]->data;

            }



            s->coded_width  = ps->sps->width;

            s->coded_height = ps->sps->height;

            s->width        = ps->sps->output_width;

            s->height       = ps->sps->output_height;

            s->format       = ps->sps->pix_fmt;

            avctx->profile  = ps->sps->ptl.general_ptl.profile_idc;

            avctx->level    = ps->sps->ptl.general_ptl.level_idc;



            if (ps->vps->vps_timing_info_present_flag) {

                num = ps->vps->vps_num_units_in_tick;

                den = ps->vps->vps_time_scale;

            } else if (ps->sps->vui.vui_timing_info_present_flag) {

                num = ps->sps->vui.vui_num_units_in_tick;

                den = ps->sps->vui.vui_time_scale;

            }



            if (num != 0 && den != 0)

                av_reduce(&avctx->framerate.den, &avctx->framerate.num,

                          num, den, 1 << 30);



            if (!sh->first_slice_in_pic_flag) {

                int slice_address_length;



                if (ps->pps->dependent_slice_segments_enabled_flag)

                    sh->dependent_slice_segment_flag = get_bits1(gb);

                else

                    sh->dependent_slice_segment_flag = 0;



                slice_address_length = av_ceil_log2_c(ps->sps->ctb_width *

                                                      ps->sps->ctb_height);

                sh->slice_segment_addr = get_bitsz(gb, slice_address_length);

                if (sh->slice_segment_addr >= ps->sps->ctb_width * ps->sps->ctb_height) {

                    av_log(avctx, AV_LOG_ERROR, "Invalid slice segment address: %u.\n",

                           sh->slice_segment_addr);

                    return AVERROR_INVALIDDATA;

                }

            } else

                sh->dependent_slice_segment_flag = 0;



            if (sh->dependent_slice_segment_flag)

                break;



            for (i = 0; i < ps->pps->num_extra_slice_header_bits; i++)

                skip_bits(gb, 1); // slice_reserved_undetermined_flag[]



            sh->slice_type = get_ue_golomb(gb);

            if (!(sh->slice_type == HEVC_SLICE_I || sh->slice_type == HEVC_SLICE_P ||

                  sh->slice_type == HEVC_SLICE_B)) {

                av_log(avctx, AV_LOG_ERROR, "Unknown slice type: %d.\n",

                       sh->slice_type);

                return AVERROR_INVALIDDATA;

            }

            s->pict_type = sh->slice_type == HEVC_SLICE_B ? AV_PICTURE_TYPE_B :

                           sh->slice_type == HEVC_SLICE_P ? AV_PICTURE_TYPE_P :

                                                       AV_PICTURE_TYPE_I;



            if (ps->pps->output_flag_present_flag)

                sh->pic_output_flag = get_bits1(gb);



            if (ps->sps->separate_colour_plane_flag)

                sh->colour_plane_id = get_bits(gb, 2);



            if (!IS_IDR(h)) {

                sh->pic_order_cnt_lsb = get_bits(gb, ps->sps->log2_max_poc_lsb);

                s->output_picture_number = h->poc = ff_hevc_compute_poc(h->ps.sps, h->pocTid0, sh->pic_order_cnt_lsb, h->nal_unit_type);

            } else

                s->output_picture_number = h->poc = 0;



            if (h->temporal_id == 0 &&

                h->nal_unit_type != HEVC_NAL_TRAIL_N &&

                h->nal_unit_type != HEVC_NAL_TSA_N &&

                h->nal_unit_type != HEVC_NAL_STSA_N &&

                h->nal_unit_type != HEVC_NAL_RADL_N &&

                h->nal_unit_type != HEVC_NAL_RASL_N &&

                h->nal_unit_type != HEVC_NAL_RADL_R &&

                h->nal_unit_type != HEVC_NAL_RASL_R)

                h->pocTid0 = h->poc;



            return 0; /* no need to evaluate the rest */

        }

    }

    /* didn't find a picture! */

    if (!is_global)

        av_log(h->avctx, AV_LOG_ERROR, "missing picture in access unit\n");

    return -1;

}
