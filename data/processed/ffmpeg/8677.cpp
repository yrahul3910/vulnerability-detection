static void avc_luma_mid_8w_msa(const uint8_t *src, int32_t src_stride,

                                uint8_t *dst, int32_t dst_stride,

                                int32_t height)

{

    uint32_t loop_cnt;

    v16i8 src0, src1, src2, src3, src4;

    v16i8 mask0, mask1, mask2;

    v8i16 hz_out0, hz_out1, hz_out2, hz_out3;

    v8i16 hz_out4, hz_out5, hz_out6, hz_out7, hz_out8;

    v8i16 dst0, dst1, dst2, dst3;

    v16u8 out0, out1;



    LD_SB3(&luma_mask_arr[0], 16, mask0, mask1, mask2);



    LD_SB5(src, src_stride, src0, src1, src2, src3, src4);

    XORI_B5_128_SB(src0, src1, src2, src3, src4);

    src += (5 * src_stride);



    hz_out0 = AVC_HORZ_FILTER_SH(src0, src0, mask0, mask1, mask2);

    hz_out1 = AVC_HORZ_FILTER_SH(src1, src1, mask0, mask1, mask2);

    hz_out2 = AVC_HORZ_FILTER_SH(src2, src2, mask0, mask1, mask2);

    hz_out3 = AVC_HORZ_FILTER_SH(src3, src3, mask0, mask1, mask2);

    hz_out4 = AVC_HORZ_FILTER_SH(src4, src4, mask0, mask1, mask2);



    for (loop_cnt = (height >> 2); loop_cnt--;) {

        LD_SB4(src, src_stride, src0, src1, src2, src3);

        XORI_B4_128_SB(src0, src1, src2, src3);

        src += (4 * src_stride);



        hz_out5 = AVC_HORZ_FILTER_SH(src0, src0, mask0, mask1, mask2);

        hz_out6 = AVC_HORZ_FILTER_SH(src1, src1, mask0, mask1, mask2);

        hz_out7 = AVC_HORZ_FILTER_SH(src2, src2, mask0, mask1, mask2);

        hz_out8 = AVC_HORZ_FILTER_SH(src3, src3, mask0, mask1, mask2);

        dst0 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out0, hz_out1, hz_out2,

                                               hz_out3, hz_out4, hz_out5);

        dst1 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out1, hz_out2, hz_out3,

                                               hz_out4, hz_out5, hz_out6);

        dst2 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out2, hz_out3, hz_out4,

                                               hz_out5, hz_out6, hz_out7);

        dst3 = AVC_CALC_DPADD_H_6PIX_2COEFF_SH(hz_out3, hz_out4, hz_out5,

                                               hz_out6, hz_out7, hz_out8);

        out0 = PCKEV_XORI128_UB(dst0, dst1);

        out1 = PCKEV_XORI128_UB(dst2, dst3);

        ST8x4_UB(out0, out1, dst, dst_stride);



        dst += (4 * dst_stride);

        hz_out3 = hz_out7;

        hz_out1 = hz_out5;

        hz_out5 = hz_out4;

        hz_out4 = hz_out8;

        hz_out2 = hz_out6;

        hz_out0 = hz_out5;

    }

}
