static void sbr_hf_g_filt_c(int (*Y)[2], const int (*X_high)[40][2],

                          const SoftFloat *g_filt, int m_max, intptr_t ixh)

{

    int m;

    int64_t accu;



    for (m = 0; m < m_max; m++) {

        int64_t r = 1LL << (22-g_filt[m].exp);

        accu = (int64_t)X_high[m][ixh][0] * ((g_filt[m].mant + 0x40)>>7);

        Y[m][0] = (int)((accu + r) >> (23-g_filt[m].exp));



        accu = (int64_t)X_high[m][ixh][1] * ((g_filt[m].mant + 0x40)>>7);

        Y[m][1] = (int)((accu + r) >> (23-g_filt[m].exp));

    }

}
