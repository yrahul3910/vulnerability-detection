static av_cold int XAVS_init(AVCodecContext *avctx)

{

    XavsContext *x4 = avctx->priv_data;



    x4->sei_size = 0;

    xavs_param_default(&x4->params);



    x4->params.pf_log               = XAVS_log;

    x4->params.p_log_private        = avctx;

    x4->params.i_keyint_max         = avctx->gop_size;

    if (avctx->bit_rate) {

        x4->params.rc.i_bitrate   = avctx->bit_rate / 1000;

        x4->params.rc.i_rc_method = XAVS_RC_ABR;

    }

    x4->params.rc.i_vbv_buffer_size = avctx->rc_buffer_size / 1000;

    x4->params.rc.i_vbv_max_bitrate = avctx->rc_max_rate    / 1000;

    x4->params.rc.b_stat_write      = avctx->flags & AV_CODEC_FLAG_PASS1;

    if (avctx->flags & AV_CODEC_FLAG_PASS2) {

        x4->params.rc.b_stat_read = 1;

    } else {

        if (x4->crf >= 0) {

            x4->params.rc.i_rc_method   = XAVS_RC_CRF;

            x4->params.rc.f_rf_constant = x4->crf;

        } else if (x4->cqp >= 0) {

            x4->params.rc.i_rc_method   = XAVS_RC_CQP;

            x4->params.rc.i_qp_constant = x4->cqp;

        }

    }



    if (x4->aud >= 0)

        x4->params.b_aud                      = x4->aud;

    if (x4->mbtree >= 0)

        x4->params.rc.b_mb_tree               = x4->mbtree;

    if (x4->direct_pred >= 0)

        x4->params.analyse.i_direct_mv_pred   = x4->direct_pred;

    if (x4->fast_pskip >= 0)

        x4->params.analyse.b_fast_pskip       = x4->fast_pskip;

    if (x4->motion_est >= 0)

        x4->params.analyse.i_me_method        = x4->motion_est;

    if (x4->mixed_refs >= 0)

        x4->params.analyse.b_mixed_references = x4->mixed_refs;

    if (x4->b_bias != INT_MIN)

        x4->params.i_bframe_bias              = x4->b_bias;

    if (x4->cplxblur >= 0)

        x4->params.rc.f_complexity_blur = x4->cplxblur;



#if FF_API_MOTION_EST

FF_DISABLE_DEPRECATION_WARNINGS

    if (x4->motion_est < 0) {

        switch (avctx->me_method) {

        case  ME_EPZS:

            x4->params.analyse.i_me_method = XAVS_ME_DIA;

            break;

        case  ME_HEX:

            x4->params.analyse.i_me_method = XAVS_ME_HEX;

            break;

        case  ME_UMH:

            x4->params.analyse.i_me_method = XAVS_ME_UMH;

            break;

        case  ME_FULL:

            x4->params.analyse.i_me_method = XAVS_ME_ESA;

            break;

        case  ME_TESA:

            x4->params.analyse.i_me_method = XAVS_ME_TESA;

            break;

        default:

            x4->params.analyse.i_me_method = XAVS_ME_HEX;

        }

    }

FF_ENABLE_DEPRECATION_WARNINGS

#endif



    x4->params.i_bframe          = avctx->max_b_frames;

    /* cabac is not included in AVS JiZhun Profile */

    x4->params.b_cabac           = 0;



    x4->params.i_bframe_adaptive = avctx->b_frame_strategy;



    avctx->has_b_frames          = !!avctx->max_b_frames;



    /* AVS doesn't allow B picture as reference */

    /* The max allowed reference frame number of B is 2 */

    x4->params.i_keyint_min      = avctx->keyint_min;

    if (x4->params.i_keyint_min > x4->params.i_keyint_max)

        x4->params.i_keyint_min = x4->params.i_keyint_max;



    x4->params.i_scenecut_threshold        = avctx->scenechange_threshold;



   // x4->params.b_deblocking_filter       = avctx->flags & AV_CODEC_FLAG_LOOP_FILTER;



    x4->params.rc.i_qp_min                 = avctx->qmin;

    x4->params.rc.i_qp_max                 = avctx->qmax;

    x4->params.rc.i_qp_step                = avctx->max_qdiff;



    x4->params.rc.f_qcompress       = avctx->qcompress; /* 0.0 => cbr, 1.0 => constant qp */

    x4->params.rc.f_qblur           = avctx->qblur;     /* temporally blur quants */



    x4->params.i_frame_reference    = avctx->refs;



    x4->params.i_width              = avctx->width;

    x4->params.i_height             = avctx->height;

    x4->params.vui.i_sar_width      = avctx->sample_aspect_ratio.num;

    x4->params.vui.i_sar_height     = avctx->sample_aspect_ratio.den;

    /* This is only used for counting the fps */

    x4->params.i_fps_num            = avctx->time_base.den;

    x4->params.i_fps_den            = avctx->time_base.num;

    x4->params.analyse.inter        = XAVS_ANALYSE_I8x8 |XAVS_ANALYSE_PSUB16x16| XAVS_ANALYSE_BSUB16x16;



    x4->params.analyse.i_me_range = avctx->me_range;

    x4->params.analyse.i_subpel_refine    = avctx->me_subpel_quality;



    x4->params.analyse.b_chroma_me        = avctx->me_cmp & FF_CMP_CHROMA;

    /* AVS P2 only enables 8x8 transform */

    x4->params.analyse.b_transform_8x8    = 1; //avctx->flags2 & AV_CODEC_FLAG2_8X8DCT;



    x4->params.analyse.i_trellis          = avctx->trellis;

    x4->params.analyse.i_noise_reduction  = avctx->noise_reduction;



    if (avctx->level > 0)

        x4->params.i_level_idc = avctx->level;



    if (avctx->bit_rate > 0)

        x4->params.rc.f_rate_tolerance =

            (float)avctx->bit_rate_tolerance / avctx->bit_rate;



    if ((avctx->rc_buffer_size) &&

        (avctx->rc_initial_buffer_occupancy <= avctx->rc_buffer_size)) {

        x4->params.rc.f_vbv_buffer_init =

            (float)avctx->rc_initial_buffer_occupancy / avctx->rc_buffer_size;

    } else

        x4->params.rc.f_vbv_buffer_init = 0.9;



    /* TAG:do we have MB tree RC method */

    /* what is the RC method we are now using? Default NO */

    x4->params.rc.f_ip_factor             = 1 / fabs(avctx->i_quant_factor);

    x4->params.rc.f_pb_factor             = avctx->b_quant_factor;

    x4->params.analyse.i_chroma_qp_offset = avctx->chromaoffset;



    x4->params.analyse.b_psnr = avctx->flags & AV_CODEC_FLAG_PSNR;

    x4->params.i_log_level    = XAVS_LOG_DEBUG;

    x4->params.i_threads      = avctx->thread_count;

    x4->params.b_interlaced   = avctx->flags & AV_CODEC_FLAG_INTERLACED_DCT;



    if (avctx->flags & AV_CODEC_FLAG_GLOBAL_HEADER)

        x4->params.b_repeat_headers = 0;



    x4->enc = xavs_encoder_open(&x4->params);

    if (!x4->enc)

        return -1;



    if (!(x4->pts_buffer = av_mallocz((avctx->max_b_frames+1) * sizeof(*x4->pts_buffer))))

        return AVERROR(ENOMEM);



    /* TAG: Do we have GLOBAL HEADER in AVS */

    /* We Have PPS and SPS in AVS */

    if (avctx->flags & AV_CODEC_FLAG_GLOBAL_HEADER) {

        xavs_nal_t *nal;

        int nnal, s, i, size;

        uint8_t *p;



        s = xavs_encoder_headers(x4->enc, &nal, &nnal);



        avctx->extradata = p = av_malloc(s);

        for (i = 0; i < nnal; i++) {

            /* Don't put the SEI in extradata. */

            if (nal[i].i_type == NAL_SEI) {

                x4->sei = av_malloc( 5 + nal[i].i_payload * 4 / 3 );

                if (xavs_nal_encode(x4->sei, &x4->sei_size, 1, nal + i) < 0)

                    return -1;



                continue;

            }

            size = xavs_nal_encode(p, &s, 1, nal + i);

            if (size < 0)

                return -1;

            p += size;

        }

        avctx->extradata_size = p - avctx->extradata;

    }

    return 0;

}
