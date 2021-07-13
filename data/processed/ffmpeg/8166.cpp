void ff_er_frame_end(ERContext *s)

{

    int *linesize = s->cur_pic->f.linesize;

    int i, mb_x, mb_y, error, error_type, dc_error, mv_error, ac_error;

    int distance;

    int threshold_part[4] = { 100, 100, 100 };

    int threshold = 50;

    int is_intra_likely;

    int size = s->b8_stride * 2 * s->mb_height;



    /* We do not support ER of field pictures yet,

     * though it should not crash if enabled. */

    if (!s->avctx->error_concealment || s->error_count == 0            ||

        s->avctx->lowres                                               ||

        s->avctx->hwaccel                                              ||

        s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU          ||

        !s->cur_pic || s->cur_pic->field_picture                               ||

        s->error_count == 3 * s->mb_width *

                          (s->avctx->skip_top + s->avctx->skip_bottom)) {

        return;

    }

    for (mb_x = 0; mb_x < s->mb_width; mb_x++) {

        int status = s->error_status_table[mb_x + (s->mb_height - 1) * s->mb_stride];

        if (status != 0x7F)

            break;

    }



    if (   mb_x == s->mb_width

        && s->avctx->codec_id == AV_CODEC_ID_MPEG2VIDEO

        && (s->avctx->height&16)

        && s->error_count == 3 * s->mb_width * (s->avctx->skip_top + s->avctx->skip_bottom + 1)

    ) {

        av_log(s->avctx, AV_LOG_DEBUG, "ignoring last missing slice\n");

        return;

    }



    if (s->last_pic) {

        if (s->last_pic->f.width  != s->cur_pic->f.width  ||

            s->last_pic->f.height != s->cur_pic->f.height ||

            s->last_pic->f.format != s->cur_pic->f.format) {

            av_log(s->avctx, AV_LOG_WARNING, "Cannot use previous picture in error concealment\n");

            s->last_pic = NULL;

        }

    }

    if (s->next_pic) {

        if (s->next_pic->f.width  != s->cur_pic->f.width  ||

            s->next_pic->f.height != s->cur_pic->f.height ||

            s->next_pic->f.format != s->cur_pic->f.format) {

            av_log(s->avctx, AV_LOG_WARNING, "Cannot use next picture in error concealment\n");

            s->next_pic = NULL;

        }

    }



    if (s->cur_pic->motion_val[0] == NULL) {

        av_log(s->avctx, AV_LOG_ERROR, "Warning MVs not available\n");



        for (i = 0; i < 2; i++) {

            s->cur_pic->ref_index_buf[i]  = av_buffer_allocz(s->mb_stride * s->mb_height * 4 * sizeof(uint8_t));

            s->cur_pic->motion_val_buf[i] = av_buffer_allocz((size + 4) * 2 * sizeof(uint16_t));

            if (!s->cur_pic->ref_index_buf[i] || !s->cur_pic->motion_val_buf[i])

                break;

            s->cur_pic->ref_index[i]  = s->cur_pic->ref_index_buf[i]->data;

            s->cur_pic->motion_val[i] = (int16_t (*)[2])s->cur_pic->motion_val_buf[i]->data + 4;

        }

        if (i < 2) {

            for (i = 0; i < 2; i++) {

                av_buffer_unref(&s->cur_pic->ref_index_buf[i]);

                av_buffer_unref(&s->cur_pic->motion_val_buf[i]);

                s->cur_pic->ref_index[i]  = NULL;

                s->cur_pic->motion_val[i] = NULL;

            }

            return;

        }

    }



    if (s->avctx->debug & FF_DEBUG_ER) {

        for (mb_y = 0; mb_y < s->mb_height; mb_y++) {

            for (mb_x = 0; mb_x < s->mb_width; mb_x++) {

                int status = s->error_status_table[mb_x + mb_y * s->mb_stride];



                av_log(s->avctx, AV_LOG_DEBUG, "%2X ", status);

            }

            av_log(s->avctx, AV_LOG_DEBUG, "\n");

        }

    }



#if 1

    /* handle overlapping slices */

    for (error_type = 1; error_type <= 3; error_type++) {

        int end_ok = 0;



        for (i = s->mb_num - 1; i >= 0; i--) {

            const int mb_xy = s->mb_index2xy[i];

            int error       = s->error_status_table[mb_xy];



            if (error & (1 << error_type))

                end_ok = 1;

            if (error & (8 << error_type))

                end_ok = 1;



            if (!end_ok)

                s->error_status_table[mb_xy] |= 1 << error_type;



            if (error & VP_START)

                end_ok = 0;

        }

    }

#endif

#if 1

    /* handle slices with partitions of different length */

    if (s->partitioned_frame) {

        int end_ok = 0;



        for (i = s->mb_num - 1; i >= 0; i--) {

            const int mb_xy = s->mb_index2xy[i];

            int error       = s->error_status_table[mb_xy];



            if (error & ER_AC_END)

                end_ok = 0;

            if ((error & ER_MV_END) ||

                (error & ER_DC_END) ||

                (error & ER_AC_ERROR))

                end_ok = 1;



            if (!end_ok)

                s->error_status_table[mb_xy]|= ER_AC_ERROR;



            if (error & VP_START)

                end_ok = 0;

        }

    }

#endif

    /* handle missing slices */

    if (s->avctx->err_recognition & AV_EF_EXPLODE) {

        int end_ok = 1;



        // FIXME + 100 hack

        for (i = s->mb_num - 2; i >= s->mb_width + 100; i--) {

            const int mb_xy = s->mb_index2xy[i];

            int error1 = s->error_status_table[mb_xy];

            int error2 = s->error_status_table[s->mb_index2xy[i + 1]];



            if (error1 & VP_START)

                end_ok = 1;



            if (error2 == (VP_START | ER_MB_ERROR | ER_MB_END) &&

                error1 != (VP_START | ER_MB_ERROR | ER_MB_END) &&

                ((error1 & ER_AC_END) || (error1 & ER_DC_END) ||

                (error1 & ER_MV_END))) {

                // end & uninit

                end_ok = 0;

            }



            if (!end_ok)

                s->error_status_table[mb_xy] |= ER_MB_ERROR;

        }

    }



#if 1

    /* backward mark errors */

    distance = 9999999;

    for (error_type = 1; error_type <= 3; error_type++) {

        for (i = s->mb_num - 1; i >= 0; i--) {

            const int mb_xy = s->mb_index2xy[i];

            int       error = s->error_status_table[mb_xy];



            if (!s->mbskip_table[mb_xy]) // FIXME partition specific

                distance++;

            if (error & (1 << error_type))

                distance = 0;



            if (s->partitioned_frame) {

                if (distance < threshold_part[error_type - 1])

                    s->error_status_table[mb_xy] |= 1 << error_type;

            } else {

                if (distance < threshold)

                    s->error_status_table[mb_xy] |= 1 << error_type;

            }



            if (error & VP_START)

                distance = 9999999;

        }

    }

#endif



    /* forward mark errors */

    error = 0;

    for (i = 0; i < s->mb_num; i++) {

        const int mb_xy = s->mb_index2xy[i];

        int old_error   = s->error_status_table[mb_xy];



        if (old_error & VP_START) {

            error = old_error & ER_MB_ERROR;

        } else {

            error |= old_error & ER_MB_ERROR;

            s->error_status_table[mb_xy] |= error;

        }

    }

#if 1

    /* handle not partitioned case */

    if (!s->partitioned_frame) {

        for (i = 0; i < s->mb_num; i++) {

            const int mb_xy = s->mb_index2xy[i];

            error = s->error_status_table[mb_xy];

            if (error & ER_MB_ERROR)

                error |= ER_MB_ERROR;

            s->error_status_table[mb_xy] = error;

        }

    }

#endif



    dc_error = ac_error = mv_error = 0;

    for (i = 0; i < s->mb_num; i++) {

        const int mb_xy = s->mb_index2xy[i];

        error = s->error_status_table[mb_xy];

        if (error & ER_DC_ERROR)

            dc_error++;

        if (error & ER_AC_ERROR)

            ac_error++;

        if (error & ER_MV_ERROR)

            mv_error++;

    }

    av_log(s->avctx, AV_LOG_INFO, "concealing %d DC, %d AC, %d MV errors in %c frame\n",

           dc_error, ac_error, mv_error, av_get_picture_type_char(s->cur_pic->f.pict_type));



    is_intra_likely = is_intra_more_likely(s);



    /* set unknown mb-type to most likely */

    for (i = 0; i < s->mb_num; i++) {

        const int mb_xy = s->mb_index2xy[i];

        error = s->error_status_table[mb_xy];

        if (!((error & ER_DC_ERROR) && (error & ER_MV_ERROR)))

            continue;



        if (is_intra_likely)

            s->cur_pic->mb_type[mb_xy] = MB_TYPE_INTRA4x4;

        else

            s->cur_pic->mb_type[mb_xy] = MB_TYPE_16x16 | MB_TYPE_L0;

    }



    // change inter to intra blocks if no reference frames are available

    if (!(s->last_pic && s->last_pic->f.data[0]) &&

        !(s->next_pic && s->next_pic->f.data[0]))

        for (i = 0; i < s->mb_num; i++) {

            const int mb_xy = s->mb_index2xy[i];

            if (!IS_INTRA(s->cur_pic->mb_type[mb_xy]))

                s->cur_pic->mb_type[mb_xy] = MB_TYPE_INTRA4x4;

        }



    /* handle inter blocks with damaged AC */

    for (mb_y = 0; mb_y < s->mb_height; mb_y++) {

        for (mb_x = 0; mb_x < s->mb_width; mb_x++) {

            const int mb_xy   = mb_x + mb_y * s->mb_stride;

            const int mb_type = s->cur_pic->mb_type[mb_xy];

            const int dir     = !(s->last_pic && s->last_pic->f.data[0]);

            const int mv_dir  = dir ? MV_DIR_BACKWARD : MV_DIR_FORWARD;

            int mv_type;



            error = s->error_status_table[mb_xy];



            if (IS_INTRA(mb_type))

                continue; // intra

            if (error & ER_MV_ERROR)

                continue; // inter with damaged MV

            if (!(error & ER_AC_ERROR))

                continue; // undamaged inter



            if (IS_8X8(mb_type)) {

                int mb_index = mb_x * 2 + mb_y * 2 * s->b8_stride;

                int j;

                mv_type = MV_TYPE_8X8;

                for (j = 0; j < 4; j++) {

                    s->mv[0][j][0] = s->cur_pic->motion_val[dir][mb_index + (j & 1) + (j >> 1) * s->b8_stride][0];

                    s->mv[0][j][1] = s->cur_pic->motion_val[dir][mb_index + (j & 1) + (j >> 1) * s->b8_stride][1];

                }

            } else {

                mv_type     = MV_TYPE_16X16;

                s->mv[0][0][0] = s->cur_pic->motion_val[dir][mb_x * 2 + mb_y * 2 * s->b8_stride][0];

                s->mv[0][0][1] = s->cur_pic->motion_val[dir][mb_x * 2 + mb_y * 2 * s->b8_stride][1];

            }



            s->decode_mb(s->opaque, 0 /* FIXME h264 partitioned slices need this set */,

                         mv_dir, mv_type, &s->mv, mb_x, mb_y, 0, 0);

        }

    }



    /* guess MVs */

    if (s->cur_pic->f.pict_type == AV_PICTURE_TYPE_B) {

        for (mb_y = 0; mb_y < s->mb_height; mb_y++) {

            for (mb_x = 0; mb_x < s->mb_width; mb_x++) {

                int       xy      = mb_x * 2 + mb_y * 2 * s->b8_stride;

                const int mb_xy   = mb_x + mb_y * s->mb_stride;

                const int mb_type = s->cur_pic->mb_type[mb_xy];

                int mv_dir = MV_DIR_FORWARD | MV_DIR_BACKWARD;



                error = s->error_status_table[mb_xy];



                if (IS_INTRA(mb_type))

                    continue;

                if (!(error & ER_MV_ERROR))

                    continue; // inter with undamaged MV

                if (!(error & ER_AC_ERROR))

                    continue; // undamaged inter



                if (!(s->last_pic && s->last_pic->f.data[0]))

                    mv_dir &= ~MV_DIR_FORWARD;

                if (!(s->next_pic && s->next_pic->f.data[0]))

                    mv_dir &= ~MV_DIR_BACKWARD;



                if (s->pp_time) {

                    int time_pp = s->pp_time;

                    int time_pb = s->pb_time;



                    av_assert0(s->avctx->codec_id != AV_CODEC_ID_H264);

                    ff_thread_await_progress(&s->next_pic->tf, mb_y, 0);



                    s->mv[0][0][0] = s->next_pic->motion_val[0][xy][0] *  time_pb            / time_pp;

                    s->mv[0][0][1] = s->next_pic->motion_val[0][xy][1] *  time_pb            / time_pp;

                    s->mv[1][0][0] = s->next_pic->motion_val[0][xy][0] * (time_pb - time_pp) / time_pp;

                    s->mv[1][0][1] = s->next_pic->motion_val[0][xy][1] * (time_pb - time_pp) / time_pp;

                } else {

                    s->mv[0][0][0] = 0;

                    s->mv[0][0][1] = 0;

                    s->mv[1][0][0] = 0;

                    s->mv[1][0][1] = 0;

                }



                s->decode_mb(s->opaque, 0, mv_dir, MV_TYPE_16X16, &s->mv,

                             mb_x, mb_y, 0, 0);

            }

        }

    } else

        guess_mv(s);



#if FF_API_XVMC

FF_DISABLE_DEPRECATION_WARNINGS

    /* the filters below are not XvMC compatible, skip them */

    if (CONFIG_MPEG_XVMC_DECODER && s->avctx->xvmc_acceleration)

        goto ec_clean;

FF_ENABLE_DEPRECATION_WARNINGS

#endif /* FF_API_XVMC */

    /* fill DC for inter blocks */

    for (mb_y = 0; mb_y < s->mb_height; mb_y++) {

        for (mb_x = 0; mb_x < s->mb_width; mb_x++) {

            int dc, dcu, dcv, y, n;

            int16_t *dc_ptr;

            uint8_t *dest_y, *dest_cb, *dest_cr;

            const int mb_xy   = mb_x + mb_y * s->mb_stride;

            const int mb_type = s->cur_pic->mb_type[mb_xy];



            error = s->error_status_table[mb_xy];



            if (IS_INTRA(mb_type) && s->partitioned_frame)

                continue;

            // if (error & ER_MV_ERROR)

            //     continue; // inter data damaged FIXME is this good?



            dest_y  = s->cur_pic->f.data[0] + mb_x * 16 + mb_y * 16 * linesize[0];

            dest_cb = s->cur_pic->f.data[1] + mb_x *  8 + mb_y *  8 * linesize[1];

            dest_cr = s->cur_pic->f.data[2] + mb_x *  8 + mb_y *  8 * linesize[2];



            dc_ptr = &s->dc_val[0][mb_x * 2 + mb_y * 2 * s->b8_stride];

            for (n = 0; n < 4; n++) {

                dc = 0;

                for (y = 0; y < 8; y++) {

                    int x;

                    for (x = 0; x < 8; x++)

                       dc += dest_y[x + (n & 1) * 8 +

                             (y + (n >> 1) * 8) * linesize[0]];

                }

                dc_ptr[(n & 1) + (n >> 1) * s->b8_stride] = (dc + 4) >> 3;

            }



            dcu = dcv = 0;

            for (y = 0; y < 8; y++) {

                int x;

                for (x = 0; x < 8; x++) {

                    dcu += dest_cb[x + y * linesize[1]];

                    dcv += dest_cr[x + y * linesize[2]];

                }

            }

            s->dc_val[1][mb_x + mb_y * s->mb_stride] = (dcu + 4) >> 3;

            s->dc_val[2][mb_x + mb_y * s->mb_stride] = (dcv + 4) >> 3;

        }

    }

#if 1

    /* guess DC for damaged blocks */

    guess_dc(s, s->dc_val[0], s->mb_width*2, s->mb_height*2, s->b8_stride, 1);

    guess_dc(s, s->dc_val[1], s->mb_width  , s->mb_height  , s->mb_stride, 0);

    guess_dc(s, s->dc_val[2], s->mb_width  , s->mb_height  , s->mb_stride, 0);

#endif



    /* filter luma DC */

    filter181(s->dc_val[0], s->mb_width * 2, s->mb_height * 2, s->b8_stride);



#if 1

    /* render DC only intra */

    for (mb_y = 0; mb_y < s->mb_height; mb_y++) {

        for (mb_x = 0; mb_x < s->mb_width; mb_x++) {

            uint8_t *dest_y, *dest_cb, *dest_cr;

            const int mb_xy   = mb_x + mb_y * s->mb_stride;

            const int mb_type = s->cur_pic->mb_type[mb_xy];



            error = s->error_status_table[mb_xy];



            if (IS_INTER(mb_type))

                continue;

            if (!(error & ER_AC_ERROR))

                continue; // undamaged



            dest_y  = s->cur_pic->f.data[0] + mb_x * 16 + mb_y * 16 * linesize[0];

            dest_cb = s->cur_pic->f.data[1] + mb_x *  8 + mb_y *  8 * linesize[1];

            dest_cr = s->cur_pic->f.data[2] + mb_x *  8 + mb_y *  8 * linesize[2];



            put_dc(s, dest_y, dest_cb, dest_cr, mb_x, mb_y);

        }

    }

#endif



    if (s->avctx->error_concealment & FF_EC_DEBLOCK) {

        /* filter horizontal block boundaries */

        h_block_filter(s, s->cur_pic->f.data[0], s->mb_width * 2,

                       s->mb_height * 2, linesize[0], 1);

        h_block_filter(s, s->cur_pic->f.data[1], s->mb_width,

                       s->mb_height, linesize[1], 0);

        h_block_filter(s, s->cur_pic->f.data[2], s->mb_width,

                       s->mb_height, linesize[2], 0);



        /* filter vertical block boundaries */

        v_block_filter(s, s->cur_pic->f.data[0], s->mb_width * 2,

                       s->mb_height * 2, linesize[0], 1);

        v_block_filter(s, s->cur_pic->f.data[1], s->mb_width,

                       s->mb_height, linesize[1], 0);

        v_block_filter(s, s->cur_pic->f.data[2], s->mb_width,

                       s->mb_height, linesize[2], 0);

    }



ec_clean:

    /* clean a few tables */

    for (i = 0; i < s->mb_num; i++) {

        const int mb_xy = s->mb_index2xy[i];

        int       error = s->error_status_table[mb_xy];



        if (s->cur_pic->f.pict_type != AV_PICTURE_TYPE_B &&

            (error & (ER_DC_ERROR | ER_MV_ERROR | ER_AC_ERROR))) {

            s->mbskip_table[mb_xy] = 0;

        }

        s->mbintra_table[mb_xy] = 1;

    }

    s->cur_pic = NULL;

    s->next_pic    = NULL;

    s->last_pic    = NULL;

}
