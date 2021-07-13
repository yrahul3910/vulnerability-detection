static inline void idct_col(int16_t *blk, const uint8_t *quant)

{

    int t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, tA, tB, tC, tD, tE, tF;

    int t10, t11, t12, t13;

    int s0, s1, s2, s3, s4, s5, s6, s7;



    s0 = (int) blk[0 * 8] * quant[0 * 8];

    s1 = (int) blk[1 * 8] * quant[1 * 8];

    s2 = (int) blk[2 * 8] * quant[2 * 8];

    s3 = (int) blk[3 * 8] * quant[3 * 8];

    s4 = (int) blk[4 * 8] * quant[4 * 8];

    s5 = (int) blk[5 * 8] * quant[5 * 8];

    s6 = (int) blk[6 * 8] * quant[6 * 8];

    s7 = (int) blk[7 * 8] * quant[7 * 8];



    t0  =  (s3 * 19266 + s5 * 12873) >> 15;

    t1  =  (s5 * 19266 - s3 * 12873) >> 15;

    t2  = ((s7 * 4520  + s1 * 22725) >> 15) - t0;

    t3  = ((s1 * 4520  - s7 * 22725) >> 15) - t1;

    t4  = t0 * 2 + t2;

    t5  = t1 * 2 + t3;

    t6  = t2 - t3;

    t7  = t3 * 2 + t6;

    t8  = (t6 * 11585) >> 14;

    t9  = (t7 * 11585) >> 14;

    tA  = (s2 * 8867 - s6 * 21407) >> 14;

    tB  = (s6 * 8867 + s2 * 21407) >> 14;

    tC  = (s0 >> 1) - (s4 >> 1);

    tD  = (s4 >> 1) * 2 + tC;

    tE  = tC - (tA >> 1);

    tF  = tD - (tB >> 1);

    t10 = tF - t5;

    t11 = tE - t8;

    t12 = tE + (tA >> 1) * 2 - t9;

    t13 = tF + (tB >> 1) * 2 - t4;



    blk[0 * 8] = t13 + t4 * 2;

    blk[1 * 8] = t12 + t9 * 2;

    blk[2 * 8] = t11 + t8 * 2;

    blk[3 * 8] = t10 + t5 * 2;

    blk[4 * 8] = t10;

    blk[5 * 8] = t11;

    blk[6 * 8] = t12;

    blk[7 * 8] = t13;

}
