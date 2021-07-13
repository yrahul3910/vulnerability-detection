static void avc_h_loop_filter_luma_mbaff_msa(uint8_t *in, int32_t stride,

                                             int32_t alpha_in, int32_t beta_in,

                                             int8_t *tc0)

{

    uint8_t *data = in;

    uint32_t out0, out1, out2, out3;

    uint64_t load;

    uint32_t tc_val;

    v16u8 alpha, beta;

    v16i8 inp0 = { 0 };

    v16i8 inp1 = { 0 };

    v16i8 inp2 = { 0 };

    v16i8 inp3 = { 0 };

    v16i8 inp4 = { 0 };

    v16i8 inp5 = { 0 };

    v16i8 inp6 = { 0 };

    v16i8 inp7 = { 0 };

    v16i8 src0, src1, src2, src3;

    v8i16 src4, src5, src6, src7;

    v16u8 p0_asub_q0, p1_asub_p0, q1_asub_q0, p2_asub_p0, q2_asub_q0;

    v16u8 is_less_than, is_less_than_alpha, is_less_than_beta;

    v16u8 is_less_than_beta1, is_less_than_beta2;

    v8i16 tc, tc_orig_r, tc_plus1;

    v16u8 is_tc_orig1, is_tc_orig2, tc_orig = { 0 };

    v8i16 p0_ilvr_q0, p0_add_q0, q0_sub_p0, p1_sub_q1;

    v8u16 src2_r, src3_r;

    v8i16 p2_r, p1_r, q2_r, q1_r;

    v16u8 p2, q2, p0, q0;

    v4i32 dst0, dst1;

    v16i8 zeros = { 0 };



    alpha = (v16u8) __msa_fill_b(alpha_in);

    beta = (v16u8) __msa_fill_b(beta_in);



    if (tc0[0] < 0) {

        data += (2 * stride);

    } else {

        load = LOAD_DWORD(data - 3);

        inp0 = (v16i8) __msa_insert_d((v2i64) inp0, 0, load);

        load = LOAD_DWORD(data - 3 + stride);

        inp1 = (v16i8) __msa_insert_d((v2i64) inp1, 0, load);

        data += (2 * stride);

    }



    if (tc0[1] < 0) {

        data += (2 * stride);

    } else {

        load = LOAD_DWORD(data - 3);

        inp2 = (v16i8) __msa_insert_d((v2i64) inp2, 0, load);

        load = LOAD_DWORD(data - 3 + stride);

        inp3 = (v16i8) __msa_insert_d((v2i64) inp3, 0, load);

        data += (2 * stride);

    }



    if (tc0[2] < 0) {

        data += (2 * stride);

    } else {

        load = LOAD_DWORD(data - 3);

        inp4 = (v16i8) __msa_insert_d((v2i64) inp4, 0, load);

        load = LOAD_DWORD(data - 3 + stride);

        inp5 = (v16i8) __msa_insert_d((v2i64) inp5, 0, load);

        data += (2 * stride);

    }



    if (tc0[3] < 0) {

        data += (2 * stride);

    } else {

        load = LOAD_DWORD(data - 3);

        inp6 = (v16i8) __msa_insert_d((v2i64) inp6, 0, load);

        load = LOAD_DWORD(data - 3 + stride);

        inp7 = (v16i8) __msa_insert_d((v2i64) inp7, 0, load);

        data += (2 * stride);

    }



    src0 = __msa_ilvr_b(inp1, inp0);

    src1 = __msa_ilvr_b(inp3, inp2);

    src2 = __msa_ilvr_b(inp5, inp4);

    src3 = __msa_ilvr_b(inp7, inp6);

    src4 = __msa_ilvr_h((v8i16) src1, (v8i16) src0);

    src5 = __msa_ilvl_h((v8i16) src1, (v8i16) src0);

    src6 = __msa_ilvr_h((v8i16) src3, (v8i16) src2);

    src7 = __msa_ilvl_h((v8i16) src3, (v8i16) src2);

    src0 = (v16i8) __msa_ilvr_w((v4i32) src6, (v4i32) src4);

    src1 = __msa_sldi_b(zeros, (v16i8) src0, 8);

    src2 = (v16i8) __msa_ilvl_w((v4i32) src6, (v4i32) src4);

    src3 = __msa_sldi_b(zeros, (v16i8) src2, 8);

    src4 = (v8i16) __msa_ilvr_w((v4i32) src7, (v4i32) src5);

    src5 = (v8i16) __msa_sldi_b(zeros, (v16i8) src4, 8);



    p0_asub_q0 = __msa_asub_u_b((v16u8) src2, (v16u8) src3);

    p1_asub_p0 = __msa_asub_u_b((v16u8) src1, (v16u8) src2);

    q1_asub_q0 = __msa_asub_u_b((v16u8) src4, (v16u8) src3);

    p2_asub_p0 = __msa_asub_u_b((v16u8) src0, (v16u8) src2);

    q2_asub_q0 = __msa_asub_u_b((v16u8) src5, (v16u8) src3);



    is_less_than_alpha = (p0_asub_q0 < alpha);

    is_less_than_beta = (p1_asub_p0 < beta);

    is_less_than = is_less_than_alpha & is_less_than_beta;

    is_less_than_beta = (q1_asub_q0 < beta);

    is_less_than = is_less_than_beta & is_less_than;



    is_less_than_beta1 = (p2_asub_p0 < beta);

    is_less_than_beta2 = (q2_asub_q0 < beta);



    p0_ilvr_q0 = (v8i16) __msa_ilvr_b((v16i8) src3, (v16i8) src2);

    p0_add_q0 = (v8i16) __msa_hadd_u_h((v16u8) p0_ilvr_q0, (v16u8) p0_ilvr_q0);

    p0_add_q0 = __msa_srari_h(p0_add_q0, 1);



    p2_r = (v8i16) __msa_ilvr_b(zeros, src0);

    p1_r = (v8i16) __msa_ilvr_b(zeros, src1);

    p2_r += p0_add_q0;

    p2_r >>= 1;

    p2_r -= p1_r;

    q2_r = (v8i16) __msa_ilvr_b(zeros, (v16i8) src5);

    q1_r = (v8i16) __msa_ilvr_b(zeros, (v16i8) src4);

    q2_r += p0_add_q0;

    q2_r >>= 1;

    q2_r -= q1_r;



    tc_val = LOAD_WORD(tc0);

    tc_orig = (v16u8) __msa_insert_w((v4i32) tc_orig, 0, tc_val);

    tc_orig = (v16u8) __msa_ilvr_b((v16i8) tc_orig, (v16i8) tc_orig);

    is_tc_orig1 = tc_orig;

    is_tc_orig2 = tc_orig;

    tc_orig_r = (v8i16) __msa_ilvr_b(zeros, (v16i8) tc_orig);

    tc = tc_orig_r;



    p2_r = CLIP_MIN_TO_MAX_H(p2_r, -tc_orig_r, tc_orig_r);

    q2_r = CLIP_MIN_TO_MAX_H(q2_r, -tc_orig_r, tc_orig_r);



    p2_r += p1_r;

    q2_r += q1_r;



    p2 = (v16u8) __msa_pckev_b((v16i8) p2_r, (v16i8) p2_r);

    q2 = (v16u8) __msa_pckev_b((v16i8) q2_r, (v16i8) q2_r);



    is_tc_orig1 = (zeros < is_tc_orig1);

    is_tc_orig2 = is_tc_orig1;

    is_tc_orig1 = is_less_than_beta1 & is_tc_orig1;

    is_tc_orig2 = is_less_than_beta2 & is_tc_orig2;

    is_tc_orig1 = is_less_than & is_tc_orig1;

    is_tc_orig2 = is_less_than & is_tc_orig2;



    p2 = __msa_bmnz_v((v16u8) src1, p2, is_tc_orig1);

    q2 = __msa_bmnz_v((v16u8) src4, q2, is_tc_orig2);



    q0_sub_p0 = __msa_hsub_u_h((v16u8) p0_ilvr_q0, (v16u8) p0_ilvr_q0);

    q0_sub_p0 <<= 2;

    p1_sub_q1 = p1_r - q1_r;

    q0_sub_p0 += p1_sub_q1;

    q0_sub_p0 = __msa_srari_h(q0_sub_p0, 3);



    tc_plus1 = tc + 1;

    is_less_than_beta1 = (v16u8) __msa_ilvr_b((v16i8) is_less_than_beta1,

                                              (v16i8) is_less_than_beta1);

    tc = (v8i16) __msa_bmnz_v((v16u8) tc, (v16u8) tc_plus1, is_less_than_beta1);

    tc_plus1 = tc + 1;

    is_less_than_beta2 = (v16u8) __msa_ilvr_b((v16i8) is_less_than_beta2,

                                              (v16i8) is_less_than_beta2);

    tc = (v8i16) __msa_bmnz_v((v16u8) tc, (v16u8) tc_plus1, is_less_than_beta2);



    q0_sub_p0 = CLIP_MIN_TO_MAX_H(q0_sub_p0, -tc, tc);



    src2_r = (v8u16) __msa_ilvr_b(zeros, src2);

    src3_r = (v8u16) __msa_ilvr_b(zeros, src3);

    src2_r += q0_sub_p0;

    src3_r -= q0_sub_p0;



    src2_r = (v8u16) CLIP_UNSIGNED_CHAR_H(src2_r);

    src3_r = (v8u16) CLIP_UNSIGNED_CHAR_H(src3_r);



    p0 = (v16u8) __msa_pckev_b((v16i8) src2_r, (v16i8) src2_r);

    q0 = (v16u8) __msa_pckev_b((v16i8) src3_r, (v16i8) src3_r);



    p0 = __msa_bmnz_v((v16u8) src2, p0, is_less_than);

    q0 = __msa_bmnz_v((v16u8) src3, q0, is_less_than);



    p2 = (v16u8) __msa_ilvr_b((v16i8) p0, (v16i8) p2);

    q2 = (v16u8) __msa_ilvr_b((v16i8) q2, (v16i8) q0);

    dst0 = (v4i32) __msa_ilvr_h((v8i16) q2, (v8i16) p2);

    dst1 = (v4i32) __msa_ilvl_h((v8i16) q2, (v8i16) p2);



    data = in;



    out0 = __msa_copy_u_w(dst0, 0);

    out1 = __msa_copy_u_w(dst0, 1);

    out2 = __msa_copy_u_w(dst0, 2);

    out3 = __msa_copy_u_w(dst0, 3);



    if (tc0[0] < 0) {

        data += (2 * stride);

    } else {

        STORE_WORD((data - 2), out0);

        data += stride;

        STORE_WORD((data - 2), out1);

        data += stride;

    }



    if (tc0[1] < 0) {

        data += (2 * stride);

    } else {

        STORE_WORD((data - 2), out2);

        data += stride;

        STORE_WORD((data - 2), out3);

        data += stride;

    }



    out0 = __msa_copy_u_w(dst1, 0);

    out1 = __msa_copy_u_w(dst1, 1);

    out2 = __msa_copy_u_w(dst1, 2);

    out3 = __msa_copy_u_w(dst1, 3);



    if (tc0[2] < 0) {

        data += (2 * stride);

    } else {

        STORE_WORD((data - 2), out0);

        data += stride;

        STORE_WORD((data - 2), out1);

        data += stride;

    }



    if (tc0[3] >= 0) {

        STORE_WORD((data - 2), out2);

        data += stride;

        STORE_WORD((data - 2), out3);

    }

}
