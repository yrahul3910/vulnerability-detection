static void sr_1d97_int(int32_t *p, int i0, int i1)

{

    int i;



    if (i1 <= i0 + 1) {

        if (i0 == 1)

            p[1] = (p[1] * I_LFTG_K + (1<<16)) >> 17;

        else

            p[0] = (p[0] * I_LFTG_X + (1<<15)) >> 16;

        return;

    }



    extend97_int(p, i0, i1);



    for (i = (i0 >> 1) - 1; i < (i1 >> 1) + 2; i++)

        p[2 * i]     -= (I_LFTG_DELTA * (p[2 * i - 1] + p[2 * i + 1]) + (1 << 15)) >> 16;

    /* step 4 */

    for (i = (i0 >> 1) - 1; i < (i1 >> 1) + 1; i++)

        p[2 * i + 1] -= (I_LFTG_GAMMA * (p[2 * i]     + p[2 * i + 2]) + (1 << 15)) >> 16;

    /*step 5*/

    for (i = (i0 >> 1); i < (i1 >> 1) + 1; i++)

        p[2 * i]     += (I_LFTG_BETA  * (p[2 * i - 1] + p[2 * i + 1]) + (1 << 15)) >> 16;

    /* step 6 */

    for (i = (i0 >> 1); i < (i1 >> 1); i++)

        p[2 * i + 1] += (I_LFTG_ALPHA * (p[2 * i]     + p[2 * i + 2]) + (1 << 15)) >> 16;

}
