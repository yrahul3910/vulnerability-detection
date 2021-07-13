av_cold int MPV_encode_init(AVCodecContext *avctx)

{

    MpegEncContext *s = avctx->priv_data;

    int i;

    int chroma_h_shift, chroma_v_shift;



    MPV_encode_defaults(s);



    switch (avctx->codec_id) {

    case CODEC_ID_MPEG2VIDEO:

        if(avctx->pix_fmt != PIX_FMT_YUV420P && avctx->pix_fmt != PIX_FMT_YUV422P){

            av_log(avctx, AV_LOG_ERROR, "only YUV420 and YUV422 are supported\n");

            return -1;

        }

        break;

    case CODEC_ID_LJPEG:

    case CODEC_ID_MJPEG:

        if(avctx->pix_fmt != PIX_FMT_YUVJ420P && avctx->pix_fmt != PIX_FMT_YUVJ422P && avctx->pix_fmt != PIX_FMT_RGB32 &&

           ((avctx->pix_fmt != PIX_FMT_YUV420P && avctx->pix_fmt != PIX_FMT_YUV422P) || avctx->strict_std_compliance>FF_COMPLIANCE_INOFFICIAL)){

            av_log(avctx, AV_LOG_ERROR, "colorspace not supported in jpeg\n");

            return -1;

        }

        break;

    default:

        if(avctx->pix_fmt != PIX_FMT_YUV420P){

            av_log(avctx, AV_LOG_ERROR, "only YUV420 is supported\n");

            return -1;

        }

    }



    switch (avctx->pix_fmt) {

    case PIX_FMT_YUVJ422P:

    case PIX_FMT_YUV422P:

        s->chroma_format = CHROMA_422;

        break;

    case PIX_FMT_YUVJ420P:

    case PIX_FMT_YUV420P:

    default:

        s->chroma_format = CHROMA_420;

        break;

    }



    s->bit_rate = avctx->bit_rate;

    s->width = avctx->width;

    s->height = avctx->height;

    if(avctx->gop_size > 600 && avctx->strict_std_compliance>FF_COMPLIANCE_EXPERIMENTAL){

        av_log(avctx, AV_LOG_ERROR, "Warning keyframe interval too large! reducing it ...\n");

        avctx->gop_size=600;

    }

    s->gop_size = avctx->gop_size;

    s->avctx = avctx;

    s->flags= avctx->flags;

    s->flags2= avctx->flags2;

    s->max_b_frames= avctx->max_b_frames;

    s->codec_id= avctx->codec->id;

    s->luma_elim_threshold  = avctx->luma_elim_threshold;

    s->chroma_elim_threshold= avctx->chroma_elim_threshold;

    s->strict_std_compliance= avctx->strict_std_compliance;

    s->data_partitioning= avctx->flags & CODEC_FLAG_PART;

    s->quarter_sample= (avctx->flags & CODEC_FLAG_QPEL)!=0;

    s->mpeg_quant= avctx->mpeg_quant;

    s->rtp_mode= !!avctx->rtp_payload_size;

    s->intra_dc_precision= avctx->intra_dc_precision;

    s->user_specified_pts = AV_NOPTS_VALUE;



    if (s->gop_size <= 1) {

        s->intra_only = 1;

        s->gop_size = 12;

    } else {

        s->intra_only = 0;

    }



    s->me_method = avctx->me_method;



    /* Fixed QSCALE */

    s->fixed_qscale = !!(avctx->flags & CODEC_FLAG_QSCALE);



    s->adaptive_quant= (   s->avctx->lumi_masking

                        || s->avctx->dark_masking

                        || s->avctx->temporal_cplx_masking

                        || s->avctx->spatial_cplx_masking

                        || s->avctx->p_masking

                        || s->avctx->border_masking

                        || (s->flags&CODEC_FLAG_QP_RD))

                       && !s->fixed_qscale;



    s->obmc= !!(s->flags & CODEC_FLAG_OBMC);

    s->loop_filter= !!(s->flags & CODEC_FLAG_LOOP_FILTER);

    s->alternate_scan= !!(s->flags & CODEC_FLAG_ALT_SCAN);

    s->intra_vlc_format= !!(s->flags2 & CODEC_FLAG2_INTRA_VLC);

    s->q_scale_type= !!(s->flags2 & CODEC_FLAG2_NON_LINEAR_QUANT);



    if(avctx->rc_max_rate && !avctx->rc_buffer_size){

        av_log(avctx, AV_LOG_ERROR, "a vbv buffer size is needed, for encoding with a maximum bitrate\n");

        return -1;

    }



    if(avctx->rc_min_rate && avctx->rc_max_rate != avctx->rc_min_rate){

        av_log(avctx, AV_LOG_INFO, "Warning min_rate > 0 but min_rate != max_rate isn't recommended!\n");

    }



    if(avctx->rc_min_rate && avctx->rc_min_rate > avctx->bit_rate){

        av_log(avctx, AV_LOG_ERROR, "bitrate below min bitrate\n");

        return -1;

    }



    if(avctx->rc_max_rate && avctx->rc_max_rate < avctx->bit_rate){

        av_log(avctx, AV_LOG_INFO, "bitrate above max bitrate\n");

        return -1;

    }



    if(avctx->rc_max_rate && avctx->rc_max_rate == avctx->bit_rate && avctx->rc_max_rate != avctx->rc_min_rate){

        av_log(avctx, AV_LOG_INFO, "impossible bitrate constraints, this will fail\n");

    }



    if(avctx->rc_buffer_size && avctx->bit_rate*av_q2d(avctx->time_base) > avctx->rc_buffer_size){

        av_log(avctx, AV_LOG_ERROR, "VBV buffer too small for bitrate\n");

        return -1;

    }



    if(avctx->bit_rate*av_q2d(avctx->time_base) > avctx->bit_rate_tolerance){

        av_log(avctx, AV_LOG_ERROR, "bitrate tolerance too small for bitrate\n");

        return -1;

    }



    if(   s->avctx->rc_max_rate && s->avctx->rc_min_rate == s->avctx->rc_max_rate

       && (s->codec_id == CODEC_ID_MPEG1VIDEO || s->codec_id == CODEC_ID_MPEG2VIDEO)

       && 90000LL * (avctx->rc_buffer_size-1) > s->avctx->rc_max_rate*0xFFFFLL){



        av_log(avctx, AV_LOG_INFO, "Warning vbv_delay will be set to 0xFFFF (=VBR) as the specified vbv buffer is too large for the given bitrate!\n");

    }



    if((s->flags & CODEC_FLAG_4MV) && s->codec_id != CODEC_ID_MPEG4

       && s->codec_id != CODEC_ID_H263 && s->codec_id != CODEC_ID_H263P && s->codec_id != CODEC_ID_FLV1){

        av_log(avctx, AV_LOG_ERROR, "4MV not supported by codec\n");

        return -1;

    }



    if(s->obmc && s->avctx->mb_decision != FF_MB_DECISION_SIMPLE){

        av_log(avctx, AV_LOG_ERROR, "OBMC is only supported with simple mb decision\n");

        return -1;

    }



    if(s->obmc && s->codec_id != CODEC_ID_H263 && s->codec_id != CODEC_ID_H263P){

        av_log(avctx, AV_LOG_ERROR, "OBMC is only supported with H263(+)\n");

        return -1;

    }



    if(s->quarter_sample && s->codec_id != CODEC_ID_MPEG4){

        av_log(avctx, AV_LOG_ERROR, "qpel not supported by codec\n");

        return -1;

    }



    if(s->data_partitioning && s->codec_id != CODEC_ID_MPEG4){

        av_log(avctx, AV_LOG_ERROR, "data partitioning not supported by codec\n");

        return -1;

    }



    if(s->max_b_frames && s->codec_id != CODEC_ID_MPEG4 && s->codec_id != CODEC_ID_MPEG1VIDEO && s->codec_id != CODEC_ID_MPEG2VIDEO){

        av_log(avctx, AV_LOG_ERROR, "b frames not supported by codec\n");

        return -1;

    }



    if((s->flags & (CODEC_FLAG_INTERLACED_DCT|CODEC_FLAG_INTERLACED_ME|CODEC_FLAG_ALT_SCAN))

       && s->codec_id != CODEC_ID_MPEG4 && s->codec_id != CODEC_ID_MPEG2VIDEO){

        av_log(avctx, AV_LOG_ERROR, "interlacing not supported by codec\n");

        return -1;

    }



    if(s->mpeg_quant && s->codec_id != CODEC_ID_MPEG4){ //FIXME mpeg2 uses that too

        av_log(avctx, AV_LOG_ERROR, "mpeg2 style quantization not supported by codec\n");

        return -1;

    }



    if((s->flags & CODEC_FLAG_CBP_RD) && !avctx->trellis){

        av_log(avctx, AV_LOG_ERROR, "CBP RD needs trellis quant\n");

        return -1;

    }



    if((s->flags & CODEC_FLAG_QP_RD) && s->avctx->mb_decision != FF_MB_DECISION_RD){

        av_log(avctx, AV_LOG_ERROR, "QP RD needs mbd=2\n");

        return -1;

    }



    if(s->avctx->scenechange_threshold < 1000000000 && (s->flags & CODEC_FLAG_CLOSED_GOP)){

        av_log(avctx, AV_LOG_ERROR, "closed gop with scene change detection are not supported yet, set threshold to 1000000000\n");

        return -1;

    }



    if((s->flags2 & CODEC_FLAG2_INTRA_VLC) && s->codec_id != CODEC_ID_MPEG2VIDEO){

        av_log(avctx, AV_LOG_ERROR, "intra vlc table not supported by codec\n");

        return -1;

    }



    if(s->flags & CODEC_FLAG_LOW_DELAY){

        if (s->codec_id != CODEC_ID_MPEG2VIDEO){

            av_log(avctx, AV_LOG_ERROR, "low delay forcing is only available for mpeg2\n");

            return -1;

        }

        if (s->max_b_frames != 0){

            av_log(avctx, AV_LOG_ERROR, "b frames cannot be used with low delay\n");

            return -1;

        }

    }



    if(s->q_scale_type == 1){

        if(s->codec_id != CODEC_ID_MPEG2VIDEO){

            av_log(avctx, AV_LOG_ERROR, "non linear quant is only available for mpeg2\n");

            return -1;

        }

        if(avctx->qmax > 12){

            av_log(avctx, AV_LOG_ERROR, "non linear quant only supports qmax <= 12 currently\n");

            return -1;

        }

    }



    if(s->avctx->thread_count > 1 && s->codec_id != CODEC_ID_MPEG4

       && s->codec_id != CODEC_ID_MPEG1VIDEO && s->codec_id != CODEC_ID_MPEG2VIDEO

       && (s->codec_id != CODEC_ID_H263P || !(s->flags & CODEC_FLAG_H263P_SLICE_STRUCT))){

        av_log(avctx, AV_LOG_ERROR, "multi threaded encoding not supported by codec\n");

        return -1;

    }



    if(s->avctx->thread_count < 1){

        av_log(avctx, AV_LOG_ERROR, "automatic thread number detection not supported by codec, patch welcome\n");

        return -1;

    }



    if(s->avctx->thread_count > 1)

        s->rtp_mode= 1;



    if(!avctx->time_base.den || !avctx->time_base.num){

        av_log(avctx, AV_LOG_ERROR, "framerate not set\n");

        return -1;

    }



    i= (INT_MAX/2+128)>>8;

    if(avctx->me_threshold >= i){

        av_log(avctx, AV_LOG_ERROR, "me_threshold too large, max is %d\n", i - 1);

        return -1;

    }

    if(avctx->mb_threshold >= i){

        av_log(avctx, AV_LOG_ERROR, "mb_threshold too large, max is %d\n", i - 1);

        return -1;

    }



    if(avctx->b_frame_strategy && (avctx->flags&CODEC_FLAG_PASS2)){

        av_log(avctx, AV_LOG_INFO, "notice: b_frame_strategy only affects the first pass\n");

        avctx->b_frame_strategy = 0;

    }



    i= av_gcd(avctx->time_base.den, avctx->time_base.num);

    if(i > 1){

        av_log(avctx, AV_LOG_INFO, "removing common factors from framerate\n");

        avctx->time_base.den /= i;

        avctx->time_base.num /= i;

//        return -1;

    }



    if(s->codec_id==CODEC_ID_MJPEG){

        s->intra_quant_bias= 1<<(QUANT_BIAS_SHIFT-1); //(a + x/2)/x

        s->inter_quant_bias= 0;

    }else if(s->mpeg_quant || s->codec_id==CODEC_ID_MPEG1VIDEO || s->codec_id==CODEC_ID_MPEG2VIDEO){

        s->intra_quant_bias= 3<<(QUANT_BIAS_SHIFT-3); //(a + x*3/8)/x

        s->inter_quant_bias= 0;

    }else{

        s->intra_quant_bias=0;

        s->inter_quant_bias=-(1<<(QUANT_BIAS_SHIFT-2)); //(a - x/4)/x

    }



    if(avctx->intra_quant_bias != FF_DEFAULT_QUANT_BIAS)

        s->intra_quant_bias= avctx->intra_quant_bias;

    if(avctx->inter_quant_bias != FF_DEFAULT_QUANT_BIAS)

        s->inter_quant_bias= avctx->inter_quant_bias;



    avcodec_get_chroma_sub_sample(avctx->pix_fmt, &chroma_h_shift, &chroma_v_shift);



    if(avctx->codec_id == CODEC_ID_MPEG4 && s->avctx->time_base.den > (1<<16)-1){

        av_log(avctx, AV_LOG_ERROR, "timebase not supported by mpeg 4 standard\n");

        return -1;

    }

    s->time_increment_bits = av_log2(s->avctx->time_base.den - 1) + 1;



    switch(avctx->codec->id) {

    case CODEC_ID_MPEG1VIDEO:

        s->out_format = FMT_MPEG1;

        s->low_delay= !!(s->flags & CODEC_FLAG_LOW_DELAY);

        avctx->delay= s->low_delay ? 0 : (s->max_b_frames + 1);

        break;

    case CODEC_ID_MPEG2VIDEO:

        s->out_format = FMT_MPEG1;

        s->low_delay= !!(s->flags & CODEC_FLAG_LOW_DELAY);

        avctx->delay= s->low_delay ? 0 : (s->max_b_frames + 1);

        s->rtp_mode= 1;

        break;

    case CODEC_ID_LJPEG:

    case CODEC_ID_MJPEG:

        s->out_format = FMT_MJPEG;

        s->intra_only = 1; /* force intra only for jpeg */

        if(avctx->codec->id == CODEC_ID_MJPEG || avctx->pix_fmt != PIX_FMT_BGRA){

        s->mjpeg_vsample[0] = 2;

        s->mjpeg_vsample[1] = 2>>chroma_v_shift;

        s->mjpeg_vsample[2] = 2>>chroma_v_shift;

        s->mjpeg_hsample[0] = 2;

        s->mjpeg_hsample[1] = 2>>chroma_h_shift;

        s->mjpeg_hsample[2] = 2>>chroma_h_shift;

        }else{

            s->mjpeg_vsample[0] = s->mjpeg_hsample[0] =

            s->mjpeg_vsample[1] = s->mjpeg_hsample[1] =

            s->mjpeg_vsample[2] = s->mjpeg_hsample[2] = 1;

        }

        if (!(CONFIG_MJPEG_ENCODER || CONFIG_LJPEG_ENCODER)

            || ff_mjpeg_encode_init(s) < 0)

            return -1;

        avctx->delay=0;

        s->low_delay=1;

        break;

    case CODEC_ID_H261:

        if (!CONFIG_H261_ENCODER)  return -1;

        if (ff_h261_get_picture_format(s->width, s->height) < 0) {

            av_log(avctx, AV_LOG_ERROR, "The specified picture size of %dx%d is not valid for the H.261 codec.\nValid sizes are 176x144, 352x288\n", s->width, s->height);

            return -1;

        }

        s->out_format = FMT_H261;

        avctx->delay=0;

        s->low_delay=1;

        break;

    case CODEC_ID_H263:

        if (!CONFIG_H263_ENCODER)  return -1;

        if (h263_get_picture_format(s->width, s->height) == 7) {

            av_log(avctx, AV_LOG_INFO, "The specified picture size of %dx%d is not valid for the H.263 codec.\nValid sizes are 128x96, 176x144, 352x288, 704x576, and 1408x1152. Try H.263+.\n", s->width, s->height);

            return -1;

        }

        s->out_format = FMT_H263;

        s->obmc= (avctx->flags & CODEC_FLAG_OBMC) ? 1:0;

        avctx->delay=0;

        s->low_delay=1;

        break;

    case CODEC_ID_H263P:

        s->out_format = FMT_H263;

        s->h263_plus = 1;

        /* Fx */

        s->umvplus = (avctx->flags & CODEC_FLAG_H263P_UMV) ? 1:0;

        s->h263_aic= (avctx->flags & CODEC_FLAG_AC_PRED) ? 1:0;

        s->modified_quant= s->h263_aic;

        s->alt_inter_vlc= (avctx->flags & CODEC_FLAG_H263P_AIV) ? 1:0;

        s->obmc= (avctx->flags & CODEC_FLAG_OBMC) ? 1:0;

        s->loop_filter= (avctx->flags & CODEC_FLAG_LOOP_FILTER) ? 1:0;

        s->unrestricted_mv= s->obmc || s->loop_filter || s->umvplus;

        s->h263_slice_structured= (s->flags & CODEC_FLAG_H263P_SLICE_STRUCT) ? 1:0;



        /* /Fx */

        /* These are just to be sure */

        avctx->delay=0;

        s->low_delay=1;

        break;

    case CODEC_ID_FLV1:

        s->out_format = FMT_H263;

        s->h263_flv = 2; /* format = 1; 11-bit codes */

        s->unrestricted_mv = 1;

        s->rtp_mode=0; /* don't allow GOB */

        avctx->delay=0;

        s->low_delay=1;

        break;

    case CODEC_ID_RV10:

        s->out_format = FMT_H263;

        avctx->delay=0;

        s->low_delay=1;

        break;

    case CODEC_ID_RV20:

        s->out_format = FMT_H263;

        avctx->delay=0;

        s->low_delay=1;

        s->modified_quant=1;

        s->h263_aic=1;

        s->h263_plus=1;

        s->loop_filter=1;

        s->unrestricted_mv= s->obmc || s->loop_filter || s->umvplus;

        break;

    case CODEC_ID_MPEG4:

        s->out_format = FMT_H263;

        s->h263_pred = 1;

        s->unrestricted_mv = 1;

        s->low_delay= s->max_b_frames ? 0 : 1;

        avctx->delay= s->low_delay ? 0 : (s->max_b_frames + 1);

        break;

    case CODEC_ID_MSMPEG4V1:

        s->out_format = FMT_H263;

        s->h263_msmpeg4 = 1;

        s->h263_pred = 1;

        s->unrestricted_mv = 1;

        s->msmpeg4_version= 1;

        avctx->delay=0;

        s->low_delay=1;

        break;

    case CODEC_ID_MSMPEG4V2:

        s->out_format = FMT_H263;

        s->h263_msmpeg4 = 1;

        s->h263_pred = 1;

        s->unrestricted_mv = 1;

        s->msmpeg4_version= 2;

        avctx->delay=0;

        s->low_delay=1;

        break;

    case CODEC_ID_MSMPEG4V3:

        s->out_format = FMT_H263;

        s->h263_msmpeg4 = 1;

        s->h263_pred = 1;

        s->unrestricted_mv = 1;

        s->msmpeg4_version= 3;

        s->flipflop_rounding=1;

        avctx->delay=0;

        s->low_delay=1;

        break;

    case CODEC_ID_WMV1:

        s->out_format = FMT_H263;

        s->h263_msmpeg4 = 1;

        s->h263_pred = 1;

        s->unrestricted_mv = 1;

        s->msmpeg4_version= 4;

        s->flipflop_rounding=1;

        avctx->delay=0;

        s->low_delay=1;

        break;

    case CODEC_ID_WMV2:

        s->out_format = FMT_H263;

        s->h263_msmpeg4 = 1;

        s->h263_pred = 1;

        s->unrestricted_mv = 1;

        s->msmpeg4_version= 5;

        s->flipflop_rounding=1;

        avctx->delay=0;

        s->low_delay=1;

        break;

    default:

        return -1;

    }



    avctx->has_b_frames= !s->low_delay;



    s->encoding = 1;



    s->progressive_frame=

    s->progressive_sequence= !(avctx->flags & (CODEC_FLAG_INTERLACED_DCT|CODEC_FLAG_INTERLACED_ME|CODEC_FLAG_ALT_SCAN));



    /* init */

    if (MPV_common_init(s) < 0)

        return -1;



    if(!s->dct_quantize)

        s->dct_quantize = dct_quantize_c;

    if(!s->denoise_dct)

        s->denoise_dct = denoise_dct_c;

    s->fast_dct_quantize = s->dct_quantize;

    if(avctx->trellis)

        s->dct_quantize = dct_quantize_trellis_c;



    if((CONFIG_H263P_ENCODER || CONFIG_RV20_ENCODER) && s->modified_quant)

        s->chroma_qscale_table= ff_h263_chroma_qscale_table;



    s->quant_precision=5;



    ff_set_cmp(&s->dsp, s->dsp.ildct_cmp, s->avctx->ildct_cmp);

    ff_set_cmp(&s->dsp, s->dsp.frame_skip_cmp, s->avctx->frame_skip_cmp);



    if (CONFIG_H261_ENCODER && s->out_format == FMT_H261)

        ff_h261_encode_init(s);

    if (CONFIG_ANY_H263_ENCODER && s->out_format == FMT_H263)

        h263_encode_init(s);

    if (CONFIG_MSMPEG4_ENCODER && s->msmpeg4_version)

        ff_msmpeg4_encode_init(s);

    if ((CONFIG_MPEG1VIDEO_ENCODER || CONFIG_MPEG2VIDEO_ENCODER)

        && s->out_format == FMT_MPEG1)

        ff_mpeg1_encode_init(s);



    /* init q matrix */

    for(i=0;i<64;i++) {

        int j= s->dsp.idct_permutation[i];

        if(CONFIG_MPEG4_ENCODER && s->codec_id==CODEC_ID_MPEG4 && s->mpeg_quant){

            s->intra_matrix[j] = ff_mpeg4_default_intra_matrix[i];

            s->inter_matrix[j] = ff_mpeg4_default_non_intra_matrix[i];

        }else if(s->out_format == FMT_H263 || s->out_format == FMT_H261){

            s->intra_matrix[j] =

            s->inter_matrix[j] = ff_mpeg1_default_non_intra_matrix[i];

        }else

        { /* mpeg1/2 */

            s->intra_matrix[j] = ff_mpeg1_default_intra_matrix[i];

            s->inter_matrix[j] = ff_mpeg1_default_non_intra_matrix[i];

        }

        if(s->avctx->intra_matrix)

            s->intra_matrix[j] = s->avctx->intra_matrix[i];

        if(s->avctx->inter_matrix)

            s->inter_matrix[j] = s->avctx->inter_matrix[i];

    }



    /* precompute matrix */

    /* for mjpeg, we do include qscale in the matrix */

    if (s->out_format != FMT_MJPEG) {

        ff_convert_matrix(&s->dsp, s->q_intra_matrix, s->q_intra_matrix16,

                       s->intra_matrix, s->intra_quant_bias, avctx->qmin, 31, 1);

        ff_convert_matrix(&s->dsp, s->q_inter_matrix, s->q_inter_matrix16,

                       s->inter_matrix, s->inter_quant_bias, avctx->qmin, 31, 0);

    }



    if(ff_rate_control_init(s) < 0)

        return -1;



    return 0;

}
