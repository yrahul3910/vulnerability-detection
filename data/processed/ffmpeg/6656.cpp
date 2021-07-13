static void print_report(int is_last_report, int64_t timer_start, int64_t cur_time)

{

    char buf[1024];

    AVBPrint buf_script;

    OutputStream *ost;

    AVFormatContext *oc;

    int64_t total_size;

    AVCodecContext *enc;

    int frame_number, vid, i;

    double bitrate;

    double speed;

    int64_t pts = INT64_MIN + 1;

    static int64_t last_time = -1;

    static int qp_histogram[52];

    int hours, mins, secs, us;

    float t;



    if (!print_stats && !is_last_report && !progress_avio)

        return;



    if (!is_last_report) {

        if (last_time == -1) {

            last_time = cur_time;

            return;

        }

        if ((cur_time - last_time) < 500000)

            return;

        last_time = cur_time;

    }



    t = (cur_time-timer_start) / 1000000.0;





    oc = output_files[0]->ctx;



    total_size = avio_size(oc->pb);

    if (total_size <= 0) // FIXME improve avio_size() so it works with non seekable output too

        total_size = avio_tell(oc->pb);



    buf[0] = '\0';

    vid = 0;

    av_bprint_init(&buf_script, 0, 1);

    for (i = 0; i < nb_output_streams; i++) {

        float q = -1;

        ost = output_streams[i];

        enc = ost->enc_ctx;

        if (!ost->stream_copy)

            q = ost->quality / (float) FF_QP2LAMBDA;



        if (vid && enc->codec_type == AVMEDIA_TYPE_VIDEO) {

            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "q=%2.1f ", q);

            av_bprintf(&buf_script, "stream_%d_%d_q=%.1f\n",

                       ost->file_index, ost->index, q);

        }

        if (!vid && enc->codec_type == AVMEDIA_TYPE_VIDEO) {

            float fps;



            frame_number = ost->frame_number;

            fps = t > 1 ? frame_number / t : 0;

            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "frame=%5d fps=%3.*f q=%3.1f ",

                     frame_number, fps < 9.95, fps, q);

            av_bprintf(&buf_script, "frame=%d\n", frame_number);

            av_bprintf(&buf_script, "fps=%.1f\n", fps);

            av_bprintf(&buf_script, "stream_%d_%d_q=%.1f\n",

                       ost->file_index, ost->index, q);

            if (is_last_report)

                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "L");

            if (qp_hist) {

                int j;

                int qp = lrintf(q);

                if (qp >= 0 && qp < FF_ARRAY_ELEMS(qp_histogram))

                    qp_histogram[qp]++;

                for (j = 0; j < 32; j++)

                    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%X", (int)lrintf(log2(qp_histogram[j] + 1)));

            }



            if ((enc->flags & AV_CODEC_FLAG_PSNR) && (ost->pict_type != AV_PICTURE_TYPE_NONE || is_last_report)) {

                int j;

                double error, error_sum = 0;

                double scale, scale_sum = 0;

                double p;

                char type[3] = { 'Y','U','V' };

                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "PSNR=");

                for (j = 0; j < 3; j++) {

                    if (is_last_report) {

                        error = enc->error[j];

                        scale = enc->width * enc->height * 255.0 * 255.0 * frame_number;

                    } else {

                        error = ost->error[j];

                        scale = enc->width * enc->height * 255.0 * 255.0;

                    }

                    if (j)

                        scale /= 4;

                    error_sum += error;

                    scale_sum += scale;

                    p = psnr(error / scale);

                    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%c:%2.2f ", type[j], p);

                    av_bprintf(&buf_script, "stream_%d_%d_psnr_%c=%2.2f\n",

                               ost->file_index, ost->index, type[j] | 32, p);

                }

                p = psnr(error_sum / scale_sum);

                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "*:%2.2f ", psnr(error_sum / scale_sum));

                av_bprintf(&buf_script, "stream_%d_%d_psnr_all=%2.2f\n",

                           ost->file_index, ost->index, p);

            }

            vid = 1;

        }

        /* compute min output value */

        if (av_stream_get_end_pts(ost->st) != AV_NOPTS_VALUE)

            pts = FFMAX(pts, av_rescale_q(av_stream_get_end_pts(ost->st),

                                          ost->st->time_base, AV_TIME_BASE_Q));

        if (is_last_report)

            nb_frames_drop += ost->last_dropped;

    }



    secs = FFABS(pts) / AV_TIME_BASE;

    us = FFABS(pts) % AV_TIME_BASE;

    mins = secs / 60;

    secs %= 60;

    hours = mins / 60;

    mins %= 60;



    bitrate = pts && total_size >= 0 ? total_size * 8 / (pts / 1000.0) : -1;

    speed = t != 0.0 ? (double)pts / AV_TIME_BASE / t : -1;



    if (total_size < 0) snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),

                                 "size=N/A time=");

    else                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),

                                 "size=%8.0fkB time=", total_size / 1024.0);

    if (pts < 0)

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "-");

    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),

             "%02d:%02d:%02d.%02d ", hours, mins, secs,

             (100 * us) / AV_TIME_BASE);



    if (bitrate < 0) {

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),"bitrate=N/A");

        av_bprintf(&buf_script, "bitrate=N/A\n");

    }else{

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),"bitrate=%6.1fkbits/s", bitrate);

        av_bprintf(&buf_script, "bitrate=%6.1fkbits/s\n", bitrate);

    }



    if (total_size < 0) av_bprintf(&buf_script, "total_size=N/A\n");

    else                av_bprintf(&buf_script, "total_size=%"PRId64"\n", total_size);

    av_bprintf(&buf_script, "out_time_ms=%"PRId64"\n", pts);

    av_bprintf(&buf_script, "out_time=%02d:%02d:%02d.%06d\n",

               hours, mins, secs, us);



    if (nb_frames_dup || nb_frames_drop)

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " dup=%d drop=%d",

                nb_frames_dup, nb_frames_drop);

    av_bprintf(&buf_script, "dup_frames=%d\n", nb_frames_dup);

    av_bprintf(&buf_script, "drop_frames=%d\n", nb_frames_drop);



    if (speed < 0) {

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf)," speed=N/A");

        av_bprintf(&buf_script, "speed=N/A\n");

    } else {

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf)," speed=%4.3gx", speed);

        av_bprintf(&buf_script, "speed=%4.3gx\n", speed);

    }



    if (print_stats || is_last_report) {

        const char end = is_last_report ? '\n' : '\r';

        if (print_stats==1 && AV_LOG_INFO > av_log_get_level()) {

            fprintf(stderr, "%s    %c", buf, end);

        } else

            av_log(NULL, AV_LOG_INFO, "%s    %c", buf, end);



    fflush(stderr);

    }



    if (progress_avio) {

        av_bprintf(&buf_script, "progress=%s\n",

                   is_last_report ? "end" : "continue");

        avio_write(progress_avio, buf_script.str,

                   FFMIN(buf_script.len, buf_script.size - 1));

        avio_flush(progress_avio);

        av_bprint_finalize(&buf_script, NULL);

        if (is_last_report) {

            avio_closep(&progress_avio);

        }

    }



    if (is_last_report)

        print_final_stats(total_size);

}
