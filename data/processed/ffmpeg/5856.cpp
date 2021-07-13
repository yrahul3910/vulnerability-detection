static void avc_biwgt_4x4multiple_msa(uint8_t *src,

                                      int32_t src_stride,

                                      uint8_t *dst,

                                      int32_t dst_stride,

                                      int32_t height,

                                      int32_t log2_denom,

                                      int32_t src_weight,

                                      int32_t dst_weight,

                                      int32_t offset_in)

{

    uint8_t cnt;

    uint32_t load0, load1, load2, load3;

    v16i8 src_wgt, dst_wgt, wgt;

    v16i8 src0, src1, src2, src3;

    v16i8 dst0, dst1, dst2, dst3;

    v8i16 temp0, temp1, temp2, temp3;

    v8i16 denom, offset, add_val;

    int32_t val = 128 * (src_weight + dst_weight);



    offset_in = ((offset_in + 1) | 1) << log2_denom;



    src_wgt = __msa_fill_b(src_weight);

    dst_wgt = __msa_fill_b(dst_weight);

    offset = __msa_fill_h(offset_in);

    denom = __msa_fill_h(log2_denom + 1);

    add_val = __msa_fill_h(val);

    offset += add_val;



    wgt = __msa_ilvev_b(dst_wgt, src_wgt);



    for (cnt = height / 4; cnt--;) {

        LOAD_4WORDS_WITH_STRIDE(src, src_stride, load0, load1, load2, load3);

        src += (4 * src_stride);



        src0 = (v16i8) __msa_fill_w(load0);

        src1 = (v16i8) __msa_fill_w(load1);

        src2 = (v16i8) __msa_fill_w(load2);

        src3 = (v16i8) __msa_fill_w(load3);



        LOAD_4WORDS_WITH_STRIDE(dst, dst_stride, load0, load1, load2, load3);



        dst0 = (v16i8) __msa_fill_w(load0);

        dst1 = (v16i8) __msa_fill_w(load1);

        dst2 = (v16i8) __msa_fill_w(load2);

        dst3 = (v16i8) __msa_fill_w(load3);



        XORI_B_4VECS_SB(src0, src1, src2, src3, src0, src1, src2, src3, 128);



        XORI_B_4VECS_SB(dst0, dst1, dst2, dst3, dst0, dst1, dst2, dst3, 128);



        ILVR_B_4VECS_SH(src0, src1, src2, src3, dst0, dst1, dst2, dst3,

                        temp0, temp1, temp2, temp3);



        temp0 = __msa_dpadd_s_h(offset, wgt, (v16i8) temp0);

        temp1 = __msa_dpadd_s_h(offset, wgt, (v16i8) temp1);

        temp2 = __msa_dpadd_s_h(offset, wgt, (v16i8) temp2);

        temp3 = __msa_dpadd_s_h(offset, wgt, (v16i8) temp3);



        SRA_4VECS(temp0, temp1, temp2, temp3,

                  temp0, temp1, temp2, temp3, denom);



        temp0 = CLIP_UNSIGNED_CHAR_H(temp0);

        temp1 = CLIP_UNSIGNED_CHAR_H(temp1);

        temp2 = CLIP_UNSIGNED_CHAR_H(temp2);

        temp3 = CLIP_UNSIGNED_CHAR_H(temp3);



        PCKEV_B_STORE_4_BYTES_4(temp0, temp1, temp2, temp3, dst, dst_stride);

        dst += (4 * dst_stride);

    }

}
