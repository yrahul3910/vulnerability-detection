static void dequantization_int(int x, int y, Jpeg2000Cblk *cblk,

                               Jpeg2000Component *comp,

                               Jpeg2000T1Context *t1, Jpeg2000Band *band)

{

    int i, j;

    int w = cblk->coord[0][1] - cblk->coord[0][0];

    for (j = 0; j < (cblk->coord[1][1] - cblk->coord[1][0]); ++j) {

        int32_t *datap = &comp->i_data[(comp->coord[0][1] - comp->coord[0][0]) * (y + j) + x];

        int *src = t1->data[j];

        if (band->i_stepsize == 16384) {

            for (i = 0; i < w; ++i)

                datap[i] = src[i] / 2;

        } else {

            // This should be VERY uncommon

            for (i = 0; i < w; ++i)

                datap[i] = (src[i] * (int64_t)band->i_stepsize) / 32768;

        }

    }

}
