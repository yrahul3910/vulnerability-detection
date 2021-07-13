static void subband_transform(DCAEncContext *c, const int32_t *input)

{

    int ch, subs, i, k, j;



    for (ch = 0; ch < c->fullband_channels; ch++) {

        /* History is copied because it is also needed for PSY */

        int32_t hist[512];

        int hist_start = 0;

        const int chi = c->channel_order_tab[ch];



        for (i = 0; i < 512; i++)

            hist[i] = c->history[i][ch];



        for (subs = 0; subs < SUBBAND_SAMPLES; subs++) {

            int32_t accum[64];

            int32_t resp;

            int band;



            /* Calculate the convolutions at once */

            for (i = 0; i < 64; i++)

                accum[i] = 0;



            for (k = 0, i = hist_start, j = 0;

                    i < 512; k = (k + 1) & 63, i++, j++)

                accum[k] += mul32(hist[i], c->band_interpolation[j]);

            for (i = 0; i < hist_start; k = (k + 1) & 63, i++, j++)

                accum[k] += mul32(hist[i], c->band_interpolation[j]);



            for (k = 16; k < 32; k++)

                accum[k] = accum[k] - accum[31 - k];

            for (k = 32; k < 48; k++)

                accum[k] = accum[k] + accum[95 - k];



            for (band = 0; band < 32; band++) {

                resp = 0;

                for (i = 16; i < 48; i++) {

                    int s = (2 * band + 1) * (2 * (i + 16) + 1);

                    resp += mul32(accum[i], cos_t(s << 3)) >> 3;

                }



                c->subband[subs][band][ch] = ((band + 1) & 2) ? -resp : resp;

            }



            /* Copy in 32 new samples from input */

            for (i = 0; i < 32; i++)

                hist[i + hist_start] = input[(subs * 32 + i) * c->channels + chi];

            hist_start = (hist_start + 32) & 511;

        }

    }

}
