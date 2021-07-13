static void quantize_and_encode_band_cost_UPAIR7_mips(struct AACEncContext *s,

                                                      PutBitContext *pb, const float *in, float *out,

                                                      const float *scaled, int size, int scale_idx,

                                                      int cb, const float lambda, const float uplim,

                                                      int *bits, const float ROUNDING)

{

    const float Q34 = ff_aac_pow34sf_tab[POW_SF2_ZERO - scale_idx + SCALE_ONE_POS - SCALE_DIV_512];

    const float IQ  = ff_aac_pow2sf_tab [POW_SF2_ZERO + scale_idx - SCALE_ONE_POS + SCALE_DIV_512];

    int i;

    int qc1, qc2, qc3, qc4;



    uint8_t  *p_bits  = (uint8_t*) ff_aac_spectral_bits[cb-1];

    uint16_t *p_codes = (uint16_t*)ff_aac_spectral_codes[cb-1];

    float    *p_vec   = (float    *)ff_aac_codebook_vectors[cb-1];



    abs_pow34_v(s->scoefs, in, size);

    scaled = s->scoefs;

    for (i = 0; i < size; i += 4) {

        int curidx1, curidx2, sign1, count1, sign2, count2;

        int *in_int = (int *)&in[i];

        uint8_t v_bits;

        unsigned int v_codes;

        int t0, t1, t2, t3, t4;

        const float *vec1, *vec2;



        qc1 = scaled[i  ] * Q34 + ROUND_STANDARD;

        qc2 = scaled[i+1] * Q34 + ROUND_STANDARD;

        qc3 = scaled[i+2] * Q34 + ROUND_STANDARD;

        qc4 = scaled[i+3] * Q34 + ROUND_STANDARD;



        __asm__ volatile (

            ".set push                              \n\t"

            ".set noreorder                         \n\t"



            "ori    %[t4],      $zero,      7       \n\t"

            "ori    %[sign1],   $zero,      0       \n\t"

            "ori    %[sign2],   $zero,      0       \n\t"

            "slt    %[t0],      %[t4],      %[qc1]  \n\t"

            "slt    %[t1],      %[t4],      %[qc2]  \n\t"

            "slt    %[t2],      %[t4],      %[qc3]  \n\t"

            "slt    %[t3],      %[t4],      %[qc4]  \n\t"

            "movn   %[qc1],     %[t4],      %[t0]   \n\t"

            "movn   %[qc2],     %[t4],      %[t1]   \n\t"

            "movn   %[qc3],     %[t4],      %[t2]   \n\t"

            "movn   %[qc4],     %[t4],      %[t3]   \n\t"

            "lw     %[t0],      0(%[in_int])        \n\t"

            "lw     %[t1],      4(%[in_int])        \n\t"

            "lw     %[t2],      8(%[in_int])        \n\t"

            "lw     %[t3],      12(%[in_int])       \n\t"

            "slt    %[t0],      %[t0],      $zero   \n\t"

            "movn   %[sign1],   %[t0],      %[qc1]  \n\t"

            "slt    %[t2],      %[t2],      $zero   \n\t"

            "movn   %[sign2],   %[t2],      %[qc3]  \n\t"

            "slt    %[t1],      %[t1],      $zero   \n\t"

            "sll    %[t0],      %[sign1],   1       \n\t"

            "or     %[t0],      %[t0],      %[t1]   \n\t"

            "movn   %[sign1],   %[t0],      %[qc2]  \n\t"

            "slt    %[t3],      %[t3],      $zero   \n\t"

            "sll    %[t0],      %[sign2],   1       \n\t"

            "or     %[t0],      %[t0],      %[t3]   \n\t"

            "movn   %[sign2],   %[t0],      %[qc4]  \n\t"

            "slt    %[count1],  $zero,      %[qc1]  \n\t"

            "slt    %[t1],      $zero,      %[qc2]  \n\t"

            "slt    %[count2],  $zero,      %[qc3]  \n\t"

            "slt    %[t2],      $zero,      %[qc4]  \n\t"

            "addu   %[count1],  %[count1],  %[t1]   \n\t"

            "addu   %[count2],  %[count2],  %[t2]   \n\t"



            ".set pop                               \n\t"



            : [qc1]"+r"(qc1), [qc2]"+r"(qc2),

              [qc3]"+r"(qc3), [qc4]"+r"(qc4),

              [sign1]"=&r"(sign1), [count1]"=&r"(count1),

              [sign2]"=&r"(sign2), [count2]"=&r"(count2),

              [t0]"=&r"(t0), [t1]"=&r"(t1), [t2]"=&r"(t2), [t3]"=&r"(t3),

              [t4]"=&r"(t4)

            : [in_int]"r"(in_int)

            : "t0", "t1", "t2", "t3", "t4",

              "memory"

        );



        curidx1  = 8 * qc1;

        curidx1 += qc2;



        v_codes = (p_codes[curidx1] << count1) | sign1;

        v_bits  = p_bits[curidx1] + count1;

        put_bits(pb, v_bits, v_codes);



        curidx2  = 8 * qc3;

        curidx2 += qc4;



        v_codes = (p_codes[curidx2] << count2) | sign2;

        v_bits  = p_bits[curidx2] + count2;

        put_bits(pb, v_bits, v_codes);



        if (out) {

           vec1 = &p_vec[curidx1*2];

           vec2 = &p_vec[curidx2*2];

           out[i+0] = copysignf(vec1[0] * IQ, in[i+0]);

           out[i+1] = copysignf(vec1[1] * IQ, in[i+1]);

           out[i+2] = copysignf(vec2[0] * IQ, in[i+2]);

           out[i+3] = copysignf(vec2[1] * IQ, in[i+3]);

        }

    }

}
