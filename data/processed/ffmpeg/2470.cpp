static int synchronize_audio(VideoState *is, int nb_samples)

{

    int wanted_nb_samples = nb_samples;



    /* if not master, then we try to remove or add samples to correct the clock */

    if (get_master_sync_type(is) != AV_SYNC_AUDIO_MASTER) {

        double diff, avg_diff;

        int min_nb_samples, max_nb_samples;



        diff = get_audio_clock(is) - get_master_clock(is);



        if (fabs(diff) < AV_NOSYNC_THRESHOLD) {

            is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;

            if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {

                /* not enough measures to have a correct estimate */

                is->audio_diff_avg_count++;

            } else {

                /* estimate the A-V difference */

                avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);



                if (fabs(avg_diff) >= is->audio_diff_threshold) {

                    wanted_nb_samples = nb_samples + (int)(diff * is->audio_src.freq);

                    min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));

                    max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));

                    wanted_nb_samples = FFMIN(FFMAX(wanted_nb_samples, min_nb_samples), max_nb_samples);

                }

                av_dlog(NULL, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",

                        diff, avg_diff, wanted_nb_samples - nb_samples,

                        is->audio_clock, is->audio_diff_threshold);

            }

        } else {

            /* too big difference : may be initial PTS errors, so

               reset A-V filter */

            is->audio_diff_avg_count = 0;

            is->audio_diff_cum       = 0;

        }

    }



    return wanted_nb_samples;

}
