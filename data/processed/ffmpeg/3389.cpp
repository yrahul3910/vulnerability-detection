static void extrapolate_isf(float out[LP_ORDER_16k], float isf[LP_ORDER])

{

    float diff_isf[LP_ORDER - 2], diff_mean;

    float *diff_hi = diff_isf - LP_ORDER + 1; // diff array for extrapolated indexes

    float corr_lag[3];

    float est, scale;

    int i, i_max_corr;



    memcpy(out, isf, (LP_ORDER - 1) * sizeof(float));

    out[LP_ORDER_16k - 1] = isf[LP_ORDER - 1];



    /* Calculate the difference vector */

    for (i = 0; i < LP_ORDER - 2; i++)

        diff_isf[i] = isf[i + 1] - isf[i];



    diff_mean = 0.0;

    for (i = 2; i < LP_ORDER - 2; i++)

        diff_mean += diff_isf[i] * (1.0f / (LP_ORDER - 4));



    /* Find which is the maximum autocorrelation */

    i_max_corr = 0;

    for (i = 0; i < 3; i++) {

        corr_lag[i] = auto_correlation(diff_isf, diff_mean, i + 2);



        if (corr_lag[i] > corr_lag[i_max_corr])

            i_max_corr = i;

    }

    i_max_corr++;



    for (i = LP_ORDER - 1; i < LP_ORDER_16k - 1; i++)

        out[i] = isf[i - 1] + isf[i - 1 - i_max_corr]

                            - isf[i - 2 - i_max_corr];



    /* Calculate an estimate for ISF(18) and scale ISF based on the error */

    est   = 7965 + (out[2] - out[3] - out[4]) / 6.0;

    scale = 0.5 * (FFMIN(est, 7600) - out[LP_ORDER - 2]) /

            (out[LP_ORDER_16k - 2] - out[LP_ORDER - 2]);



    for (i = LP_ORDER - 1; i < LP_ORDER_16k - 1; i++)

        diff_hi[i] = scale * (out[i] - out[i - 1]);



    /* Stability insurance */

    for (i = LP_ORDER; i < LP_ORDER_16k - 1; i++)

        if (diff_hi[i] + diff_hi[i - 1] < 5.0) {

            if (diff_hi[i] > diff_hi[i - 1]) {

                diff_hi[i - 1] = 5.0 - diff_hi[i];

            } else

                diff_hi[i] = 5.0 - diff_hi[i - 1];

        }



    for (i = LP_ORDER - 1; i < LP_ORDER_16k - 1; i++)

        out[i] = out[i - 1] + diff_hi[i] * (1.0f / (1 << 15));



    /* Scale the ISF vector for 16000 Hz */

    for (i = 0; i < LP_ORDER_16k - 1; i++)

        out[i] *= 0.8;

}
