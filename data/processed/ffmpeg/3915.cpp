static void video_refresh(void *opaque)

{

    VideoState *is = opaque;

    VideoPicture *vp;

    double time;



    SubPicture *sp, *sp2;



    if (!is->paused && get_master_sync_type(is) == AV_SYNC_EXTERNAL_CLOCK && is->realtime)

        check_external_clock_speed(is);



    if (!display_disable && is->show_mode != SHOW_MODE_VIDEO && is->audio_st)

        video_display(is);



    if (is->video_st) {

        if (is->force_refresh)

            pictq_prev_picture(is);

retry:

        if (is->pictq_size == 0) {

            SDL_LockMutex(is->pictq_mutex);

            if (is->frame_last_dropped_pts != AV_NOPTS_VALUE && is->frame_last_dropped_pts > is->frame_last_pts) {

                update_video_pts(is, is->frame_last_dropped_pts, is->frame_last_dropped_pos, 0);

                is->frame_last_dropped_pts = AV_NOPTS_VALUE;

            }

            SDL_UnlockMutex(is->pictq_mutex);

            // nothing to do, no picture to display in the queue

        } else {

            double last_duration, duration, delay;

            /* dequeue the picture */

            vp = &is->pictq[is->pictq_rindex];



            if (vp->serial != is->videoq.serial) {

                pictq_next_picture(is);

                goto retry;

            }



            if (is->paused)

                goto display;



            /* compute nominal last_duration */

            last_duration = vp->pts - is->frame_last_pts;

            if (last_duration > 0 && last_duration < is->max_frame_duration) {

                /* if duration of the last frame was sane, update last_duration in video state */

                is->frame_last_duration = last_duration;

            }

            delay = compute_target_delay(is->frame_last_duration, is);



            time= av_gettime()/1000000.0;

            if (time < is->frame_timer + delay)

                return;



            if (delay > 0)

                is->frame_timer += delay * FFMAX(1, floor((time-is->frame_timer) / delay));



            SDL_LockMutex(is->pictq_mutex);

            update_video_pts(is, vp->pts, vp->pos, vp->serial);

            SDL_UnlockMutex(is->pictq_mutex);



            if (is->pictq_size > 1) {

                VideoPicture *nextvp = &is->pictq[(is->pictq_rindex + 1) % VIDEO_PICTURE_QUEUE_SIZE];

                duration = nextvp->pts - vp->pts;

                if(!is->step && (framedrop>0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) && time > is->frame_timer + duration){

                    is->frame_drops_late++;

                    pictq_next_picture(is);

                    goto retry;

                }

            }



            if (is->subtitle_st) {

                if (is->subtitle_stream_changed) {

                    SDL_LockMutex(is->subpq_mutex);



                    while (is->subpq_size) {

                        free_subpicture(&is->subpq[is->subpq_rindex]);



                        /* update queue size and signal for next picture */

                        if (++is->subpq_rindex == SUBPICTURE_QUEUE_SIZE)

                            is->subpq_rindex = 0;



                        is->subpq_size--;

                    }

                    is->subtitle_stream_changed = 0;



                    SDL_CondSignal(is->subpq_cond);

                    SDL_UnlockMutex(is->subpq_mutex);

                } else {

                    if (is->subpq_size > 0) {

                        sp = &is->subpq[is->subpq_rindex];



                        if (is->subpq_size > 1)

                            sp2 = &is->subpq[(is->subpq_rindex + 1) % SUBPICTURE_QUEUE_SIZE];

                        else

                            sp2 = NULL;



                        if ((is->video_current_pts > (sp->pts + ((float) sp->sub.end_display_time / 1000)))

                                || (sp2 && is->video_current_pts > (sp2->pts + ((float) sp2->sub.start_display_time / 1000))))

                        {

                            free_subpicture(sp);



                            /* update queue size and signal for next picture */

                            if (++is->subpq_rindex == SUBPICTURE_QUEUE_SIZE)

                                is->subpq_rindex = 0;



                            SDL_LockMutex(is->subpq_mutex);

                            is->subpq_size--;

                            SDL_CondSignal(is->subpq_cond);

                            SDL_UnlockMutex(is->subpq_mutex);

                        }

                    }

                }

            }



display:

            /* display picture */

            if (!display_disable && is->show_mode == SHOW_MODE_VIDEO)

                video_display(is);



            pictq_next_picture(is);



            if (is->step && !is->paused)

                stream_toggle_pause(is);

        }

    }

    is->force_refresh = 0;

    if (show_status) {

        static int64_t last_time;

        int64_t cur_time;

        int aqsize, vqsize, sqsize;

        double av_diff;



        cur_time = av_gettime();

        if (!last_time || (cur_time - last_time) >= 30000) {

            aqsize = 0;

            vqsize = 0;

            sqsize = 0;

            if (is->audio_st)

                aqsize = is->audioq.size;

            if (is->video_st)

                vqsize = is->videoq.size;

            if (is->subtitle_st)

                sqsize = is->subtitleq.size;

            av_diff = 0;

            if (is->audio_st && is->video_st)

                av_diff = get_audio_clock(is) - get_video_clock(is);

            printf("%7.2f A-V:%7.3f fd=%4d aq=%5dKB vq=%5dKB sq=%5dB f=%"PRId64"/%"PRId64"   \r",

                   get_master_clock(is),

                   av_diff,

                   is->frame_drops_early + is->frame_drops_late,

                   aqsize / 1024,

                   vqsize / 1024,

                   sqsize,

                   is->video_st ? is->video_st->codec->pts_correction_num_faulty_dts : 0,

                   is->video_st ? is->video_st->codec->pts_correction_num_faulty_pts : 0);

            fflush(stdout);

            last_time = cur_time;

        }

    }

}
