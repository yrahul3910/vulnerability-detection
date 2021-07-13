static void imdct36(INTFLOAT *out, INTFLOAT *buf, INTFLOAT *in, INTFLOAT *win)

{

    int i, j;

    INTFLOAT t0, t1, t2, t3, s0, s1, s2, s3;

    INTFLOAT tmp[18], *tmp1, *in1;



    for (i = 17; i >= 1; i--)

        in[i] += in[i-1];

    for (i = 17; i >= 3; i -= 2)

        in[i] += in[i-2];



    for (j = 0; j < 2; j++) {

        tmp1 = tmp + j;

        in1 = in + j;



        t2 = in1[2*4] + in1[2*8] - in1[2*2];



        t3 = in1[2*0] + SHR(in1[2*6],1);

        t1 = in1[2*0] - in1[2*6];

        tmp1[ 6] = t1 - SHR(t2,1);

        tmp1[16] = t1 + t2;



        t0 = MULH3(in1[2*2] + in1[2*4] ,    C2, 2);

        t1 = MULH3(in1[2*4] - in1[2*8] , -2*C8, 1);

        t2 = MULH3(in1[2*2] + in1[2*8] ,   -C4, 2);



        tmp1[10] = t3 - t0 - t2;

        tmp1[ 2] = t3 + t0 + t1;

        tmp1[14] = t3 + t2 - t1;



        tmp1[ 4] = MULH3(in1[2*5] + in1[2*7] - in1[2*1], -C3, 2);

        t2 = MULH3(in1[2*1] + in1[2*5],    C1, 2);

        t3 = MULH3(in1[2*5] - in1[2*7], -2*C7, 1);

        t0 = MULH3(in1[2*3], C3, 2);



        t1 = MULH3(in1[2*1] + in1[2*7],   -C5, 2);



        tmp1[ 0] = t2 + t3 + t0;

        tmp1[12] = t2 + t1 - t0;

        tmp1[ 8] = t3 - t1 - t0;

    }



    i = 0;

    for (j = 0; j < 4; j++) {

        t0 = tmp[i];

        t1 = tmp[i + 2];

        s0 = t1 + t0;

        s2 = t1 - t0;



        t2 = tmp[i + 1];

        t3 = tmp[i + 3];

        s1 = MULH3(t3 + t2, icos36h[    j], 2);

        s3 = MULLx(t3 - t2, icos36 [8 - j], FRAC_BITS);



        t0 = s0 + s1;

        t1 = s0 - s1;

        out[(9 + j) * SBLIMIT] = MULH3(t1, win[     9 + j], 1) + buf[4*(9 + j)];

        out[(8 - j) * SBLIMIT] = MULH3(t1, win[     8 - j], 1) + buf[4*(8 - j)];

        buf[4 * ( 9 + j     )] = MULH3(t0, win[MDCT_BUF_SIZE/2 + 9 + j], 1);

        buf[4 * ( 8 - j     )] = MULH3(t0, win[MDCT_BUF_SIZE/2 + 8 - j], 1);



        t0 = s2 + s3;

        t1 = s2 - s3;

        out[(9 + 8 - j) * SBLIMIT] = MULH3(t1, win[     9 + 8 - j], 1) + buf[4*(9 + 8 - j)];

        out[         j  * SBLIMIT] = MULH3(t1, win[             j], 1) + buf[4*(        j)];

        buf[4 * ( 9 + 8 - j     )] = MULH3(t0, win[MDCT_BUF_SIZE/2 + 9 + 8 - j], 1);

        buf[4 * (         j     )] = MULH3(t0, win[MDCT_BUF_SIZE/2         + j], 1);

        i += 4;

    }



    s0 = tmp[16];

    s1 = MULH3(tmp[17], icos36h[4], 2);

    t0 = s0 + s1;

    t1 = s0 - s1;

    out[(9 + 4) * SBLIMIT] = MULH3(t1, win[     9 + 4], 1) + buf[4*(9 + 4)];

    out[(8 - 4) * SBLIMIT] = MULH3(t1, win[     8 - 4], 1) + buf[4*(8 - 4)];

    buf[4 * ( 9 + 4     )] = MULH3(t0, win[MDCT_BUF_SIZE/2 + 9 + 4], 1);

    buf[4 * ( 8 - 4     )] = MULH3(t0, win[MDCT_BUF_SIZE/2 + 8 - 4], 1);

}
