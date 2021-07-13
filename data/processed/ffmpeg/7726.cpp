int ff_h263_decode_picture_header(MpegEncContext *s)

{

    int format, width, height, i;

    uint32_t startcode;



    align_get_bits(&s->gb);



    if (show_bits(&s->gb, 2) == 2 && s->avctx->frame_number == 0) {

         av_log(s->avctx, AV_LOG_WARNING, "Header looks like RTP instead of H.263\n");

    }



    startcode= get_bits(&s->gb, 22-8);



    for(i= get_bits_left(&s->gb); i>24; i-=8) {

        startcode = ((startcode << 8) | get_bits(&s->gb, 8)) & 0x003FFFFF;



        if(startcode == 0x20)

            break;

    }



    if (startcode != 0x20) {

        av_log(s->avctx, AV_LOG_ERROR, "Bad picture start code\n");

        return -1;

    }

    /* temporal reference */

    i = get_bits(&s->gb, 8); /* picture timestamp */

    if( (s->picture_number&~0xFF)+i < s->picture_number)

        i+= 256;

    s->picture_number= (s->picture_number&~0xFF) + i;



    /* PTYPE starts here */

    if (get_bits1(&s->gb) != 1) {

        /* marker */

        av_log(s->avctx, AV_LOG_ERROR, "Bad marker\n");

        return -1;

    }

    if (get_bits1(&s->gb) != 0) {

        av_log(s->avctx, AV_LOG_ERROR, "Bad H263 id\n");

        return -1;      /* h263 id */

    }

    skip_bits1(&s->gb);         /* split screen off */

    skip_bits1(&s->gb);         /* camera  off */

    skip_bits1(&s->gb);         /* freeze picture release off */



    format = get_bits(&s->gb, 3);

    /*

        0    forbidden

        1    sub-QCIF

        10   QCIF

        7       extended PTYPE (PLUSPTYPE)

    */



    if (format != 7 && format != 6) {

        s->h263_plus = 0;

        /* H.263v1 */

        width = ff_h263_format[format][0];

        height = ff_h263_format[format][1];

        if (!width)

            return -1;



        s->pict_type = AV_PICTURE_TYPE_I + get_bits1(&s->gb);



        s->h263_long_vectors = get_bits1(&s->gb);



        if (get_bits1(&s->gb) != 0) {

            av_log(s->avctx, AV_LOG_ERROR, "H263 SAC not supported\n");

            return -1; /* SAC: off */

        }

        s->obmc= get_bits1(&s->gb); /* Advanced prediction mode */

        s->unrestricted_mv = s->h263_long_vectors || s->obmc;



        s->pb_frame = get_bits1(&s->gb);

        s->chroma_qscale= s->qscale = get_bits(&s->gb, 5);

        skip_bits1(&s->gb); /* Continuous Presence Multipoint mode: off */



        s->width = width;

        s->height = height;

        s->avctx->sample_aspect_ratio= (AVRational){12,11};

        s->avctx->framerate = (AVRational){ 30000, 1001 };

    } else {

        int ufep;



        /* H.263v2 */

        s->h263_plus = 1;

        ufep = get_bits(&s->gb, 3); /* Update Full Extended PTYPE */



        /* ufep other than 0 and 1 are reserved */

        if (ufep == 1) {

            /* OPPTYPE */

            format = get_bits(&s->gb, 3);

            av_dlog(s->avctx, "ufep=1, format: %d\n", format);

            s->custom_pcf= get_bits1(&s->gb);

            s->umvplus = get_bits1(&s->gb); /* Unrestricted Motion Vector */

            if (get_bits1(&s->gb) != 0) {

                av_log(s->avctx, AV_LOG_ERROR, "Syntax-based Arithmetic Coding (SAC) not supported\n");

            }

            s->obmc= get_bits1(&s->gb); /* Advanced prediction mode */

            s->h263_aic = get_bits1(&s->gb); /* Advanced Intra Coding (AIC) */

            s->loop_filter= get_bits1(&s->gb);

            s->unrestricted_mv = s->umvplus || s->obmc || s->loop_filter;

            if(s->avctx->lowres)

                s->loop_filter = 0;



            s->h263_slice_structured= get_bits1(&s->gb);

            if (get_bits1(&s->gb) != 0) {

                av_log(s->avctx, AV_LOG_ERROR, "Reference Picture Selection not supported\n");

            }

            if (get_bits1(&s->gb) != 0) {

                av_log(s->avctx, AV_LOG_ERROR, "Independent Segment Decoding not supported\n");

            }

            s->alt_inter_vlc= get_bits1(&s->gb);

            s->modified_quant= get_bits1(&s->gb);

            if(s->modified_quant)

                s->chroma_qscale_table= ff_h263_chroma_qscale_table;



            skip_bits(&s->gb, 1); /* Prevent start code emulation */



            skip_bits(&s->gb, 3); /* Reserved */

        } else if (ufep != 0) {

            av_log(s->avctx, AV_LOG_ERROR, "Bad UFEP type (%d)\n", ufep);

            return -1;

        }



        /* MPPTYPE */

        s->pict_type = get_bits(&s->gb, 3);

        switch(s->pict_type){

        case 0: s->pict_type= AV_PICTURE_TYPE_I;break;

        case 1: s->pict_type= AV_PICTURE_TYPE_P;break;

        case 2: s->pict_type= AV_PICTURE_TYPE_P;s->pb_frame = 3;break;

        case 3: s->pict_type= AV_PICTURE_TYPE_B;break;

        case 7: s->pict_type= AV_PICTURE_TYPE_I;break; //ZYGO

        default:

            return -1;

        }

        skip_bits(&s->gb, 2);

        s->no_rounding = get_bits1(&s->gb);

        skip_bits(&s->gb, 4);



        /* Get the picture dimensions */

        if (ufep) {

            if (format == 6) {

                /* Custom Picture Format (CPFMT) */

                s->aspect_ratio_info = get_bits(&s->gb, 4);

                av_dlog(s->avctx, "aspect: %d\n", s->aspect_ratio_info);

                /* aspect ratios:

                0 - forbidden

                1 - 1:1

                2 - 12:11 (CIF 4:3)

                3 - 10:11 (525-type 4:3)

                4 - 16:11 (CIF 16:9)

                5 - 40:33 (525-type 16:9)

                6-14 - reserved

                */

                width = (get_bits(&s->gb, 9) + 1) * 4;

                skip_bits1(&s->gb);

                height = get_bits(&s->gb, 9) * 4;

                av_dlog(s->avctx, "\nH.263+ Custom picture: %dx%d\n",width,height);

                if (s->aspect_ratio_info == FF_ASPECT_EXTENDED) {

                    /* aspected dimensions */

                    s->avctx->sample_aspect_ratio.num= get_bits(&s->gb, 8);

                    s->avctx->sample_aspect_ratio.den= get_bits(&s->gb, 8);

                }else{

                    s->avctx->sample_aspect_ratio= ff_h263_pixel_aspect[s->aspect_ratio_info];

                }

            } else {

                width = ff_h263_format[format][0];

                height = ff_h263_format[format][1];

                s->avctx->sample_aspect_ratio= (AVRational){12,11};

            }

            s->avctx->sample_aspect_ratio.den <<= s->ehc_mode;

            if ((width == 0) || (height == 0))

                return -1;

            s->width = width;

            s->height = height;



            if(s->custom_pcf){

                int gcd;

                s->avctx->framerate.num  = 1800000;

                s->avctx->framerate.den  = 1000 + get_bits1(&s->gb);

                s->avctx->framerate.den *= get_bits(&s->gb, 7);

                if(s->avctx->framerate.den == 0){

                    av_log(s, AV_LOG_ERROR, "zero framerate\n");

                    return -1;

                }

                gcd= av_gcd(s->avctx->framerate.den, s->avctx->framerate.num);

                s->avctx->framerate.den /= gcd;

                s->avctx->framerate.num /= gcd;

            }else{

                s->avctx->framerate = (AVRational){ 30000, 1001 };

            }

        }



        if(s->custom_pcf){

            skip_bits(&s->gb, 2); //extended Temporal reference

        }



        if (ufep) {

            if (s->umvplus) {

                if(get_bits1(&s->gb)==0) /* Unlimited Unrestricted Motion Vectors Indicator (UUI) */

                    skip_bits1(&s->gb);

            }

            if(s->h263_slice_structured){

                if (get_bits1(&s->gb) != 0) {

                    av_log(s->avctx, AV_LOG_ERROR, "rectangular slices not supported\n");

                }

                if (get_bits1(&s->gb) != 0) {

                    av_log(s->avctx, AV_LOG_ERROR, "unordered slices not supported\n");

                }

            }

        }



        s->qscale = get_bits(&s->gb, 5);

    }



    if (s->width == 0 || s->height == 0) {

        av_log(s->avctx, AV_LOG_ERROR, "dimensions 0\n");

        return -1;

    }

    s->mb_width = (s->width  + 15) / 16;

    s->mb_height = (s->height  + 15) / 16;

    s->mb_num = s->mb_width * s->mb_height;



    if (s->pb_frame) {

        skip_bits(&s->gb, 3); /* Temporal reference for B-pictures */

        if (s->custom_pcf)

            skip_bits(&s->gb, 2); //extended Temporal reference

        skip_bits(&s->gb, 2); /* Quantization information for B-pictures */

    }



    if (s->pict_type!=AV_PICTURE_TYPE_B) {

        s->time            = s->picture_number;

        s->pp_time         = s->time - s->last_non_b_time;

        s->last_non_b_time = s->time;

    }else{

        s->time    = s->picture_number;

        s->pb_time = s->pp_time - (s->last_non_b_time - s->time);

        if (s->pp_time <=s->pb_time ||

            s->pp_time <= s->pp_time - s->pb_time ||

            s->pp_time <= 0){

            s->pp_time = 2;

            s->pb_time = 1;

        }

        ff_mpeg4_init_direct_mv(s);

    }



    /* PEI */

    if (skip_1stop_8data_bits(&s->gb) < 0)

        return AVERROR_INVALIDDATA;



    if(s->h263_slice_structured){

        if (get_bits1(&s->gb) != 1) {

            av_log(s->avctx, AV_LOG_ERROR, "SEPB1 marker missing\n");

            return -1;

        }



        ff_h263_decode_mba(s);



        if (get_bits1(&s->gb) != 1) {

            av_log(s->avctx, AV_LOG_ERROR, "SEPB2 marker missing\n");

            return -1;

        }

    }

    s->f_code = 1;



    if(s->h263_aic){

         s->y_dc_scale_table=

         s->c_dc_scale_table= ff_aic_dc_scale_table;

    }else{

        s->y_dc_scale_table=

        s->c_dc_scale_table= ff_mpeg1_dc_scale_table;

    }



        ff_h263_show_pict_info(s);

    if (s->pict_type == AV_PICTURE_TYPE_I && s->codec_tag == AV_RL32("ZYGO") && get_bits_left(&s->gb) >= 85 + 13*3*16 + 50){

        int i,j;

        for(i=0; i<85; i++) av_log(s->avctx, AV_LOG_DEBUG, "%d", get_bits1(&s->gb));

        av_log(s->avctx, AV_LOG_DEBUG, "\n");

        for(i=0; i<13; i++){

            for(j=0; j<3; j++){

                int v= get_bits(&s->gb, 8);

                v |= get_sbits(&s->gb, 8)<<8;

                av_log(s->avctx, AV_LOG_DEBUG, " %5d", v);

            }

            av_log(s->avctx, AV_LOG_DEBUG, "\n");

        }

        for(i=0; i<50; i++) av_log(s->avctx, AV_LOG_DEBUG, "%d", get_bits1(&s->gb));

    }



    return 0;

}
