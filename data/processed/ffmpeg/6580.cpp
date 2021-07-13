static void lsp2lpc(int16_t *lpc)

{

    int f1[LPC_ORDER / 2 + 1];

    int f2[LPC_ORDER / 2 + 1];

    int i, j;



    /* Calculate negative cosine */

    for (j = 0; j < LPC_ORDER; j++) {

        int index     = lpc[j] >> 7;

        int offset    = lpc[j] & 0x7f;

        int temp1     = cos_tab[index] << 16;

        int temp2     = (cos_tab[index + 1] - cos_tab[index]) *

                          ((offset << 8) + 0x80) << 1;



        lpc[j] = -(av_sat_dadd32(1 << 15, temp1 + temp2) >> 16);

    }



    /*

     * Compute sum and difference polynomial coefficients

     * (bitexact alternative to lsp2poly() in lsp.c)

     */

    /* Initialize with values in Q28 */

    f1[0] = 1 << 28;

    f1[1] = (lpc[0] << 14) + (lpc[2] << 14);

    f1[2] = lpc[0] * lpc[2] + (2 << 28);



    f2[0] = 1 << 28;

    f2[1] = (lpc[1] << 14) + (lpc[3] << 14);

    f2[2] = lpc[1] * lpc[3] + (2 << 28);



    /*

     * Calculate and scale the coefficients by 1/2 in

     * each iteration for a final scaling factor of Q25

     */

    for (i = 2; i < LPC_ORDER / 2; i++) {

        f1[i + 1] = f1[i - 1] + MULL2(f1[i], lpc[2 * i]);

        f2[i + 1] = f2[i - 1] + MULL2(f2[i], lpc[2 * i + 1]);



        for (j = i; j >= 2; j--) {

            f1[j] = MULL2(f1[j - 1], lpc[2 * i]) +

                    (f1[j] >> 1) + (f1[j - 2] >> 1);

            f2[j] = MULL2(f2[j - 1], lpc[2 * i + 1]) +

                    (f2[j] >> 1) + (f2[j - 2] >> 1);

        }



        f1[0] >>= 1;

        f2[0] >>= 1;

        f1[1] = ((lpc[2 * i]     << 16 >> i) + f1[1]) >> 1;

        f2[1] = ((lpc[2 * i + 1] << 16 >> i) + f2[1]) >> 1;

    }



    /* Convert polynomial coefficients to LPC coefficients */

    for (i = 0; i < LPC_ORDER / 2; i++) {

        int64_t ff1 = f1[i + 1] + f1[i];

        int64_t ff2 = f2[i + 1] - f2[i];



        lpc[i] = av_clipl_int32(((ff1 + ff2) << 3) + (1 << 15)) >> 16;

        lpc[LPC_ORDER - i - 1] = av_clipl_int32(((ff1 - ff2) << 3) +

                                                (1 << 15)) >> 16;

    }

}
