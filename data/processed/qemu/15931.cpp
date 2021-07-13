static void disas_xtensa_insn(DisasContext *dc)

{

#define HAS_OPTION(opt) do { \

        if (!option_enabled(dc, opt)) { \

            qemu_log("Option %d is not enabled %s:%d\n", \

                    (opt), __FILE__, __LINE__); \

            goto invalid_opcode; \

        } \

    } while (0)



#ifdef TARGET_WORDS_BIGENDIAN

#define OP0 (((b0) & 0xf0) >> 4)

#define OP1 (((b2) & 0xf0) >> 4)

#define OP2 ((b2) & 0xf)

#define RRR_R ((b1) & 0xf)

#define RRR_S (((b1) & 0xf0) >> 4)

#define RRR_T ((b0) & 0xf)

#else

#define OP0 (((b0) & 0xf))

#define OP1 (((b2) & 0xf))

#define OP2 (((b2) & 0xf0) >> 4)

#define RRR_R (((b1) & 0xf0) >> 4)

#define RRR_S (((b1) & 0xf))

#define RRR_T (((b0) & 0xf0) >> 4)

#endif



#define RRRN_R RRR_R

#define RRRN_S RRR_S

#define RRRN_T RRR_T



#define RRI8_R RRR_R

#define RRI8_S RRR_S

#define RRI8_T RRR_T

#define RRI8_IMM8 (b2)

#define RRI8_IMM8_SE ((((b2) & 0x80) ? 0xffffff00 : 0) | RRI8_IMM8)



#ifdef TARGET_WORDS_BIGENDIAN

#define RI16_IMM16 (((b1) << 8) | (b2))

#else

#define RI16_IMM16 (((b2) << 8) | (b1))

#endif



#ifdef TARGET_WORDS_BIGENDIAN

#define CALL_N (((b0) & 0xc) >> 2)

#define CALL_OFFSET ((((b0) & 0x3) << 16) | ((b1) << 8) | (b2))

#else

#define CALL_N (((b0) & 0x30) >> 4)

#define CALL_OFFSET ((((b0) & 0xc0) >> 6) | ((b1) << 2) | ((b2) << 10))

#endif

#define CALL_OFFSET_SE \

    (((CALL_OFFSET & 0x20000) ? 0xfffc0000 : 0) | CALL_OFFSET)



#define CALLX_N CALL_N

#ifdef TARGET_WORDS_BIGENDIAN

#define CALLX_M ((b0) & 0x3)

#else

#define CALLX_M (((b0) & 0xc0) >> 6)

#endif

#define CALLX_S RRR_S



#define BRI12_M CALLX_M

#define BRI12_S RRR_S

#ifdef TARGET_WORDS_BIGENDIAN

#define BRI12_IMM12 ((((b1) & 0xf) << 8) | (b2))

#else

#define BRI12_IMM12 ((((b1) & 0xf0) >> 4) | ((b2) << 4))

#endif

#define BRI12_IMM12_SE (((BRI12_IMM12 & 0x800) ? 0xfffff000 : 0) | BRI12_IMM12)



#define BRI8_M BRI12_M

#define BRI8_R RRI8_R

#define BRI8_S RRI8_S

#define BRI8_IMM8 RRI8_IMM8

#define BRI8_IMM8_SE RRI8_IMM8_SE



#define RSR_SR (b1)



    uint8_t b0 = ldub_code(dc->pc);

    uint8_t b1 = ldub_code(dc->pc + 1);

    uint8_t b2 = ldub_code(dc->pc + 2);



    static const uint32_t B4CONST[] = {

        0xffffffff, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 32, 64, 128, 256

    };



    static const uint32_t B4CONSTU[] = {

        32768, 65536, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 32, 64, 128, 256

    };



    if (OP0 >= 8) {

        dc->next_pc = dc->pc + 2;

        HAS_OPTION(XTENSA_OPTION_CODE_DENSITY);

    } else {

        dc->next_pc = dc->pc + 3;

    }



    switch (OP0) {

    case 0: /*QRST*/

        switch (OP1) {

        case 0: /*RST0*/

            switch (OP2) {

            case 0: /*ST0*/

                if ((RRR_R & 0xc) == 0x8) {

                    HAS_OPTION(XTENSA_OPTION_BOOLEAN);

                }



                switch (RRR_R) {

                case 0: /*SNM0*/

                    switch (CALLX_M) {

                    case 0: /*ILL*/

                        break;



                    case 1: /*reserved*/

                        break;



                    case 2: /*JR*/

                        switch (CALLX_N) {

                        case 0: /*RET*/

                        case 2: /*JX*/

                            gen_jump(dc, cpu_R[CALLX_S]);

                            break;



                        case 1: /*RETWw*/

                            HAS_OPTION(XTENSA_OPTION_WINDOWED_REGISTER);

                            break;



                        case 3: /*reserved*/

                            break;

                        }

                        break;



                    case 3: /*CALLX*/

                        switch (CALLX_N) {

                        case 0: /*CALLX0*/

                            {

                                TCGv_i32 tmp = tcg_temp_new_i32();

                                tcg_gen_mov_i32(tmp, cpu_R[CALLX_S]);

                                tcg_gen_movi_i32(cpu_R[0], dc->next_pc);

                                gen_jump(dc, tmp);

                                tcg_temp_free(tmp);

                            }

                            break;



                        case 1: /*CALLX4w*/

                        case 2: /*CALLX8w*/

                        case 3: /*CALLX12w*/

                            HAS_OPTION(XTENSA_OPTION_WINDOWED_REGISTER);

                            break;

                        }

                        break;

                    }

                    break;



                case 1: /*MOVSPw*/

                    HAS_OPTION(XTENSA_OPTION_WINDOWED_REGISTER);

                    break;



                case 2: /*SYNC*/

                    break;



                case 3:

                    break;



                }

                break;



            case 1: /*AND*/

                tcg_gen_and_i32(cpu_R[RRR_R], cpu_R[RRR_S], cpu_R[RRR_T]);

                break;



            case 2: /*OR*/

                tcg_gen_or_i32(cpu_R[RRR_R], cpu_R[RRR_S], cpu_R[RRR_T]);

                break;



            case 3: /*XOR*/

                tcg_gen_xor_i32(cpu_R[RRR_R], cpu_R[RRR_S], cpu_R[RRR_T]);

                break;



            case 4: /*ST1*/

                switch (RRR_R) {

                case 0: /*SSR*/

                    gen_right_shift_sar(dc, cpu_R[RRR_S]);

                    break;



                case 1: /*SSL*/

                    gen_left_shift_sar(dc, cpu_R[RRR_S]);

                    break;



                case 2: /*SSA8L*/

                    {

                        TCGv_i32 tmp = tcg_temp_new_i32();

                        tcg_gen_shli_i32(tmp, cpu_R[RRR_S], 3);

                        gen_right_shift_sar(dc, tmp);

                        tcg_temp_free(tmp);

                    }

                    break;



                case 3: /*SSA8B*/

                    {

                        TCGv_i32 tmp = tcg_temp_new_i32();

                        tcg_gen_shli_i32(tmp, cpu_R[RRR_S], 3);

                        gen_left_shift_sar(dc, tmp);

                        tcg_temp_free(tmp);

                    }

                    break;



                case 4: /*SSAI*/

                    {

                        TCGv_i32 tmp = tcg_const_i32(

                                RRR_S | ((RRR_T & 1) << 4));

                        gen_right_shift_sar(dc, tmp);

                        tcg_temp_free(tmp);

                    }

                    break;



                case 6: /*RER*/

                    break;



                case 7: /*WER*/

                    break;



                case 8: /*ROTWw*/

                    HAS_OPTION(XTENSA_OPTION_WINDOWED_REGISTER);

                    break;



                case 14: /*NSAu*/

                    HAS_OPTION(XTENSA_OPTION_MISC_OP);

                    gen_helper_nsa(cpu_R[RRR_T], cpu_R[RRR_S]);

                    break;



                case 15: /*NSAUu*/

                    HAS_OPTION(XTENSA_OPTION_MISC_OP);

                    gen_helper_nsau(cpu_R[RRR_T], cpu_R[RRR_S]);

                    break;



                default: /*reserved*/

                    break;

                }

                break;



            case 5: /*TLB*/

                break;



            case 6: /*RT0*/

                switch (RRR_S) {

                case 0: /*NEG*/

                    tcg_gen_neg_i32(cpu_R[RRR_R], cpu_R[RRR_T]);

                    break;



                case 1: /*ABS*/

                    {

                        int label = gen_new_label();

                        tcg_gen_mov_i32(cpu_R[RRR_R], cpu_R[RRR_T]);

                        tcg_gen_brcondi_i32(

                                TCG_COND_GE, cpu_R[RRR_R], 0, label);

                        tcg_gen_neg_i32(cpu_R[RRR_R], cpu_R[RRR_T]);

                        gen_set_label(label);

                    }

                    break;



                default: /*reserved*/

                    break;

                }

                break;



            case 7: /*reserved*/

                break;



            case 8: /*ADD*/

                tcg_gen_add_i32(cpu_R[RRR_R], cpu_R[RRR_S], cpu_R[RRR_T]);

                break;



            case 9: /*ADD**/

            case 10:

            case 11:

                {

                    TCGv_i32 tmp = tcg_temp_new_i32();

                    tcg_gen_shli_i32(tmp, cpu_R[RRR_S], OP2 - 8);

                    tcg_gen_add_i32(cpu_R[RRR_R], tmp, cpu_R[RRR_T]);

                    tcg_temp_free(tmp);

                }

                break;



            case 12: /*SUB*/

                tcg_gen_sub_i32(cpu_R[RRR_R], cpu_R[RRR_S], cpu_R[RRR_T]);

                break;



            case 13: /*SUB**/

            case 14:

            case 15:

                {

                    TCGv_i32 tmp = tcg_temp_new_i32();

                    tcg_gen_shli_i32(tmp, cpu_R[RRR_S], OP2 - 12);

                    tcg_gen_sub_i32(cpu_R[RRR_R], tmp, cpu_R[RRR_T]);

                    tcg_temp_free(tmp);

                }

                break;

            }

            break;



        case 1: /*RST1*/

            switch (OP2) {

            case 0: /*SLLI*/

            case 1:

                tcg_gen_shli_i32(cpu_R[RRR_R], cpu_R[RRR_S],

                        32 - (RRR_T | ((OP2 & 1) << 4)));

                break;



            case 2: /*SRAI*/

            case 3:

                tcg_gen_sari_i32(cpu_R[RRR_R], cpu_R[RRR_T],

                        RRR_S | ((OP2 & 1) << 4));

                break;



            case 4: /*SRLI*/

                tcg_gen_shri_i32(cpu_R[RRR_R], cpu_R[RRR_T], RRR_S);

                break;



            case 6: /*XSR*/

                {

                    TCGv_i32 tmp = tcg_temp_new_i32();

                    tcg_gen_mov_i32(tmp, cpu_R[RRR_T]);

                    gen_rsr(dc, cpu_R[RRR_T], RSR_SR);

                    gen_wsr(dc, RSR_SR, tmp);

                    tcg_temp_free(tmp);

                }

                break;



                /*

                 * Note: 64 bit ops are used here solely because SAR values

                 * have range 0..63

                 */

#define gen_shift_reg(cmd, reg) do { \

                    TCGv_i64 tmp = tcg_temp_new_i64(); \

                    tcg_gen_extu_i32_i64(tmp, reg); \

                    tcg_gen_##cmd##_i64(v, v, tmp); \

                    tcg_gen_trunc_i64_i32(cpu_R[RRR_R], v); \

                    tcg_temp_free_i64(v); \

                    tcg_temp_free_i64(tmp); \

                } while (0)



#define gen_shift(cmd) gen_shift_reg(cmd, cpu_SR[SAR])



            case 8: /*SRC*/

                {

                    TCGv_i64 v = tcg_temp_new_i64();

                    tcg_gen_concat_i32_i64(v, cpu_R[RRR_T], cpu_R[RRR_S]);

                    gen_shift(shr);

                }

                break;



            case 9: /*SRL*/

                if (dc->sar_5bit) {

                    tcg_gen_shr_i32(cpu_R[RRR_R], cpu_R[RRR_T], cpu_SR[SAR]);

                } else {

                    TCGv_i64 v = tcg_temp_new_i64();

                    tcg_gen_extu_i32_i64(v, cpu_R[RRR_T]);

                    gen_shift(shr);

                }

                break;



            case 10: /*SLL*/

                if (dc->sar_m32_5bit) {

                    tcg_gen_shl_i32(cpu_R[RRR_R], cpu_R[RRR_S], dc->sar_m32);

                } else {

                    TCGv_i64 v = tcg_temp_new_i64();

                    TCGv_i32 s = tcg_const_i32(32);

                    tcg_gen_sub_i32(s, s, cpu_SR[SAR]);

                    tcg_gen_andi_i32(s, s, 0x3f);

                    tcg_gen_extu_i32_i64(v, cpu_R[RRR_S]);

                    gen_shift_reg(shl, s);

                    tcg_temp_free(s);

                }

                break;



            case 11: /*SRA*/

                if (dc->sar_5bit) {

                    tcg_gen_sar_i32(cpu_R[RRR_R], cpu_R[RRR_T], cpu_SR[SAR]);

                } else {

                    TCGv_i64 v = tcg_temp_new_i64();

                    tcg_gen_ext_i32_i64(v, cpu_R[RRR_T]);

                    gen_shift(sar);

                }

                break;

#undef gen_shift

#undef gen_shift_reg



            case 12: /*MUL16U*/

                HAS_OPTION(XTENSA_OPTION_16_BIT_IMUL);

                {

                    TCGv_i32 v1 = tcg_temp_new_i32();

                    TCGv_i32 v2 = tcg_temp_new_i32();

                    tcg_gen_ext16u_i32(v1, cpu_R[RRR_S]);

                    tcg_gen_ext16u_i32(v2, cpu_R[RRR_T]);

                    tcg_gen_mul_i32(cpu_R[RRR_R], v1, v2);

                    tcg_temp_free(v2);

                    tcg_temp_free(v1);

                }

                break;



            case 13: /*MUL16S*/

                HAS_OPTION(XTENSA_OPTION_16_BIT_IMUL);

                {

                    TCGv_i32 v1 = tcg_temp_new_i32();

                    TCGv_i32 v2 = tcg_temp_new_i32();

                    tcg_gen_ext16s_i32(v1, cpu_R[RRR_S]);

                    tcg_gen_ext16s_i32(v2, cpu_R[RRR_T]);

                    tcg_gen_mul_i32(cpu_R[RRR_R], v1, v2);

                    tcg_temp_free(v2);

                    tcg_temp_free(v1);

                }

                break;



            default: /*reserved*/

                break;

            }

            break;



        case 2: /*RST2*/

            break;



        case 3: /*RST3*/

            switch (OP2) {

            case 0: /*RSR*/

                gen_rsr(dc, cpu_R[RRR_T], RSR_SR);

                break;



            case 1: /*WSR*/

                gen_wsr(dc, RSR_SR, cpu_R[RRR_T]);

                break;



            case 2: /*SEXTu*/

                HAS_OPTION(XTENSA_OPTION_MISC_OP);

                {

                    int shift = 24 - RRR_T;



                    if (shift == 24) {

                        tcg_gen_ext8s_i32(cpu_R[RRR_R], cpu_R[RRR_S]);

                    } else if (shift == 16) {

                        tcg_gen_ext16s_i32(cpu_R[RRR_R], cpu_R[RRR_S]);

                    } else {

                        TCGv_i32 tmp = tcg_temp_new_i32();

                        tcg_gen_shli_i32(tmp, cpu_R[RRR_S], shift);

                        tcg_gen_sari_i32(cpu_R[RRR_R], tmp, shift);

                        tcg_temp_free(tmp);

                    }

                }

                break;



            case 3: /*CLAMPSu*/

                HAS_OPTION(XTENSA_OPTION_MISC_OP);

                {

                    TCGv_i32 tmp1 = tcg_temp_new_i32();

                    TCGv_i32 tmp2 = tcg_temp_new_i32();

                    int label = gen_new_label();



                    tcg_gen_sari_i32(tmp1, cpu_R[RRR_S], 24 - RRR_T);

                    tcg_gen_xor_i32(tmp2, tmp1, cpu_R[RRR_S]);

                    tcg_gen_andi_i32(tmp2, tmp2, 0xffffffff << (RRR_T + 7));

                    tcg_gen_mov_i32(cpu_R[RRR_R], cpu_R[RRR_S]);

                    tcg_gen_brcondi_i32(TCG_COND_EQ, tmp2, 0, label);



                    tcg_gen_sari_i32(tmp1, cpu_R[RRR_S], 31);

                    tcg_gen_xori_i32(cpu_R[RRR_R], tmp1,

                            0xffffffff >> (25 - RRR_T));



                    gen_set_label(label);



                    tcg_temp_free(tmp1);

                    tcg_temp_free(tmp2);

                }

                break;



            case 4: /*MINu*/

            case 5: /*MAXu*/

            case 6: /*MINUu*/

            case 7: /*MAXUu*/

                HAS_OPTION(XTENSA_OPTION_MISC_OP);

                {

                    static const TCGCond cond[] = {

                        TCG_COND_LE,

                        TCG_COND_GE,

                        TCG_COND_LEU,

                        TCG_COND_GEU

                    };

                    int label = gen_new_label();



                    if (RRR_R != RRR_T) {

                        tcg_gen_mov_i32(cpu_R[RRR_R], cpu_R[RRR_S]);

                        tcg_gen_brcond_i32(cond[OP2 - 4],

                                cpu_R[RRR_S], cpu_R[RRR_T], label);

                        tcg_gen_mov_i32(cpu_R[RRR_R], cpu_R[RRR_T]);

                    } else {

                        tcg_gen_brcond_i32(cond[OP2 - 4],

                                cpu_R[RRR_T], cpu_R[RRR_S], label);

                        tcg_gen_mov_i32(cpu_R[RRR_R], cpu_R[RRR_S]);

                    }

                    gen_set_label(label);

                }

                break;



            case 8: /*MOVEQZ*/

            case 9: /*MOVNEZ*/

            case 10: /*MOVLTZ*/

            case 11: /*MOVGEZ*/

                {

                    static const TCGCond cond[] = {

                        TCG_COND_NE,

                        TCG_COND_EQ,

                        TCG_COND_GE,

                        TCG_COND_LT

                    };

                    int label = gen_new_label();

                    tcg_gen_brcondi_i32(cond[OP2 - 8], cpu_R[RRR_T], 0, label);

                    tcg_gen_mov_i32(cpu_R[RRR_R], cpu_R[RRR_S]);

                    gen_set_label(label);

                }

                break;



            case 12: /*MOVFp*/

                HAS_OPTION(XTENSA_OPTION_BOOLEAN);

                break;



            case 13: /*MOVTp*/

                HAS_OPTION(XTENSA_OPTION_BOOLEAN);

                break;



            case 14: /*RUR*/

                {

                    int st = (RRR_S << 4) + RRR_T;

                    if (uregnames[st]) {

                        tcg_gen_mov_i32(cpu_R[RRR_R], cpu_UR[st]);

                    } else {

                        qemu_log("RUR %d not implemented, ", st);

                    }

                }

                break;



            case 15: /*WUR*/

                {

                    if (uregnames[RSR_SR]) {

                        tcg_gen_mov_i32(cpu_UR[RSR_SR], cpu_R[RRR_T]);

                    } else {

                        qemu_log("WUR %d not implemented, ", RSR_SR);

                    }

                }

                break;



            }

            break;



        case 4: /*EXTUI*/

        case 5:

            {

                int shiftimm = RRR_S | (OP1 << 4);

                int maskimm = (1 << (OP2 + 1)) - 1;



                TCGv_i32 tmp = tcg_temp_new_i32();

                tcg_gen_shri_i32(tmp, cpu_R[RRR_T], shiftimm);

                tcg_gen_andi_i32(cpu_R[RRR_R], tmp, maskimm);

                tcg_temp_free(tmp);

            }

            break;



        case 6: /*CUST0*/

            break;



        case 7: /*CUST1*/

            break;



        case 8: /*LSCXp*/

            HAS_OPTION(XTENSA_OPTION_COPROCESSOR);

            break;



        case 9: /*LSC4*/

            break;



        case 10: /*FP0*/

            HAS_OPTION(XTENSA_OPTION_FP_COPROCESSOR);

            break;



        case 11: /*FP1*/

            HAS_OPTION(XTENSA_OPTION_FP_COPROCESSOR);

            break;



        default: /*reserved*/

            break;

        }

        break;



    case 1: /*L32R*/

        {

            TCGv_i32 tmp = tcg_const_i32(

                    (0xfffc0000 | (RI16_IMM16 << 2)) +

                    ((dc->pc + 3) & ~3));



            /* no ext L32R */



            tcg_gen_qemu_ld32u(cpu_R[RRR_T], tmp, 0);

            tcg_temp_free(tmp);

        }

        break;



    case 2: /*LSAI*/

#define gen_load_store(type, shift) do { \

            TCGv_i32 addr = tcg_temp_new_i32(); \

            tcg_gen_addi_i32(addr, cpu_R[RRI8_S], RRI8_IMM8 << shift); \

            tcg_gen_qemu_##type(cpu_R[RRI8_T], addr, 0); \

            tcg_temp_free(addr); \

        } while (0)



        switch (RRI8_R) {

        case 0: /*L8UI*/

            gen_load_store(ld8u, 0);

            break;



        case 1: /*L16UI*/

            gen_load_store(ld16u, 1);

            break;



        case 2: /*L32I*/

            gen_load_store(ld32u, 2);

            break;



        case 4: /*S8I*/

            gen_load_store(st8, 0);

            break;



        case 5: /*S16I*/

            gen_load_store(st16, 1);

            break;



        case 6: /*S32I*/

            gen_load_store(st32, 2);

            break;



        case 7: /*CACHEc*/

            break;



        case 9: /*L16SI*/

            gen_load_store(ld16s, 1);

            break;



        case 10: /*MOVI*/

            tcg_gen_movi_i32(cpu_R[RRI8_T],

                    RRI8_IMM8 | (RRI8_S << 8) |

                    ((RRI8_S & 0x8) ? 0xfffff000 : 0));

            break;



        case 11: /*L32AIy*/

            HAS_OPTION(XTENSA_OPTION_MP_SYNCHRO);

            gen_load_store(ld32u, 2); /*TODO acquire?*/

            break;



        case 12: /*ADDI*/

            tcg_gen_addi_i32(cpu_R[RRI8_T], cpu_R[RRI8_S], RRI8_IMM8_SE);

            break;



        case 13: /*ADDMI*/

            tcg_gen_addi_i32(cpu_R[RRI8_T], cpu_R[RRI8_S], RRI8_IMM8_SE << 8);

            break;



        case 14: /*S32C1Iy*/

            HAS_OPTION(XTENSA_OPTION_MP_SYNCHRO);

            {

                int label = gen_new_label();

                TCGv_i32 tmp = tcg_temp_local_new_i32();

                TCGv_i32 addr = tcg_temp_local_new_i32();



                tcg_gen_mov_i32(tmp, cpu_R[RRI8_T]);

                tcg_gen_addi_i32(addr, cpu_R[RRI8_S], RRI8_IMM8 << 2);

                tcg_gen_qemu_ld32u(cpu_R[RRI8_T], addr, 0);

                tcg_gen_brcond_i32(TCG_COND_NE, cpu_R[RRI8_T],

                        cpu_SR[SCOMPARE1], label);



                tcg_gen_qemu_st32(tmp, addr, 0);



                gen_set_label(label);

                tcg_temp_free(addr);

                tcg_temp_free(tmp);

            }

            break;



        case 15: /*S32RIy*/

            HAS_OPTION(XTENSA_OPTION_MP_SYNCHRO);

            gen_load_store(st32, 2); /*TODO release?*/

            break;



        default: /*reserved*/

            break;

        }

        break;

#undef gen_load_store



    case 3: /*LSCIp*/

        HAS_OPTION(XTENSA_OPTION_COPROCESSOR);

        break;



    case 4: /*MAC16d*/

        HAS_OPTION(XTENSA_OPTION_MAC16);

        break;



    case 5: /*CALLN*/

        switch (CALL_N) {

        case 0: /*CALL0*/

            tcg_gen_movi_i32(cpu_R[0], dc->next_pc);

            gen_jumpi(dc, (dc->pc & ~3) + (CALL_OFFSET_SE << 2) + 4, 0);

            break;



        case 1: /*CALL4w*/

        case 2: /*CALL8w*/

        case 3: /*CALL12w*/

            HAS_OPTION(XTENSA_OPTION_WINDOWED_REGISTER);

            break;

        }

        break;



    case 6: /*SI*/

        switch (CALL_N) {

        case 0: /*J*/

            gen_jumpi(dc, dc->pc + 4 + CALL_OFFSET_SE, 0);

            break;



        case 1: /*BZ*/

            {

                static const TCGCond cond[] = {

                    TCG_COND_EQ, /*BEQZ*/

                    TCG_COND_NE, /*BNEZ*/

                    TCG_COND_LT, /*BLTZ*/

                    TCG_COND_GE, /*BGEZ*/

                };



                gen_brcondi(dc, cond[BRI12_M & 3], cpu_R[BRI12_S], 0,

                        4 + BRI12_IMM12_SE);

            }

            break;



        case 2: /*BI0*/

            {

                static const TCGCond cond[] = {

                    TCG_COND_EQ, /*BEQI*/

                    TCG_COND_NE, /*BNEI*/

                    TCG_COND_LT, /*BLTI*/

                    TCG_COND_GE, /*BGEI*/

                };



                gen_brcondi(dc, cond[BRI8_M & 3],

                        cpu_R[BRI8_S], B4CONST[BRI8_R], 4 + BRI8_IMM8_SE);

            }

            break;



        case 3: /*BI1*/

            switch (BRI8_M) {

            case 0: /*ENTRYw*/

                HAS_OPTION(XTENSA_OPTION_WINDOWED_REGISTER);

                break;



            case 1: /*B1*/

                switch (BRI8_R) {

                case 0: /*BFp*/

                    HAS_OPTION(XTENSA_OPTION_BOOLEAN);

                    break;



                case 1: /*BTp*/

                    HAS_OPTION(XTENSA_OPTION_BOOLEAN);

                    break;



                case 8: /*LOOP*/

                    break;



                case 9: /*LOOPNEZ*/

                    break;



                case 10: /*LOOPGTZ*/

                    break;



                default: /*reserved*/

                    break;



                }

                break;



            case 2: /*BLTUI*/

            case 3: /*BGEUI*/

                gen_brcondi(dc, BRI8_M == 2 ? TCG_COND_LTU : TCG_COND_GEU,

                        cpu_R[BRI8_S], B4CONSTU[BRI8_R], 4 + BRI8_IMM8_SE);

                break;

            }

            break;



        }

        break;



    case 7: /*B*/

        {

            TCGCond eq_ne = (RRI8_R & 8) ? TCG_COND_NE : TCG_COND_EQ;



            switch (RRI8_R & 7) {

            case 0: /*BNONE*/ /*BANY*/

                {

                    TCGv_i32 tmp = tcg_temp_new_i32();

                    tcg_gen_and_i32(tmp, cpu_R[RRI8_S], cpu_R[RRI8_T]);

                    gen_brcondi(dc, eq_ne, tmp, 0, 4 + RRI8_IMM8_SE);

                    tcg_temp_free(tmp);

                }

                break;



            case 1: /*BEQ*/ /*BNE*/

            case 2: /*BLT*/ /*BGE*/

            case 3: /*BLTU*/ /*BGEU*/

                {

                    static const TCGCond cond[] = {

                        [1] = TCG_COND_EQ,

                        [2] = TCG_COND_LT,

                        [3] = TCG_COND_LTU,

                        [9] = TCG_COND_NE,

                        [10] = TCG_COND_GE,

                        [11] = TCG_COND_GEU,

                    };

                    gen_brcond(dc, cond[RRI8_R], cpu_R[RRI8_S], cpu_R[RRI8_T],

                            4 + RRI8_IMM8_SE);

                }

                break;



            case 4: /*BALL*/ /*BNALL*/

                {

                    TCGv_i32 tmp = tcg_temp_new_i32();

                    tcg_gen_and_i32(tmp, cpu_R[RRI8_S], cpu_R[RRI8_T]);

                    gen_brcond(dc, eq_ne, tmp, cpu_R[RRI8_T],

                            4 + RRI8_IMM8_SE);

                    tcg_temp_free(tmp);

                }

                break;



            case 5: /*BBC*/ /*BBS*/

                {

                    TCGv_i32 bit = tcg_const_i32(1);

                    TCGv_i32 tmp = tcg_temp_new_i32();

                    tcg_gen_andi_i32(tmp, cpu_R[RRI8_T], 0x1f);

                    tcg_gen_shl_i32(bit, bit, tmp);

                    tcg_gen_and_i32(tmp, cpu_R[RRI8_S], bit);

                    gen_brcondi(dc, eq_ne, tmp, 0, 4 + RRI8_IMM8_SE);

                    tcg_temp_free(tmp);

                    tcg_temp_free(bit);

                }

                break;



            case 6: /*BBCI*/ /*BBSI*/

            case 7:

                {

                    TCGv_i32 tmp = tcg_temp_new_i32();

                    tcg_gen_andi_i32(tmp, cpu_R[RRI8_S],

                            1 << (((RRI8_R & 1) << 4) | RRI8_T));

                    gen_brcondi(dc, eq_ne, tmp, 0, 4 + RRI8_IMM8_SE);

                    tcg_temp_free(tmp);

                }

                break;



            }

        }

        break;



#define gen_narrow_load_store(type) do { \

            TCGv_i32 addr = tcg_temp_new_i32(); \

            tcg_gen_addi_i32(addr, cpu_R[RRRN_S], RRRN_R << 2); \

            tcg_gen_qemu_##type(cpu_R[RRRN_T], addr, 0); \

            tcg_temp_free(addr); \

        } while (0)



    case 8: /*L32I.Nn*/

        gen_narrow_load_store(ld32u);

        break;



    case 9: /*S32I.Nn*/

        gen_narrow_load_store(st32);

        break;

#undef gen_narrow_load_store



    case 10: /*ADD.Nn*/

        tcg_gen_add_i32(cpu_R[RRRN_R], cpu_R[RRRN_S], cpu_R[RRRN_T]);

        break;



    case 11: /*ADDI.Nn*/

        tcg_gen_addi_i32(cpu_R[RRRN_R], cpu_R[RRRN_S], RRRN_T ? RRRN_T : -1);

        break;



    case 12: /*ST2n*/

        if (RRRN_T < 8) { /*MOVI.Nn*/

            tcg_gen_movi_i32(cpu_R[RRRN_S],

                    RRRN_R | (RRRN_T << 4) |

                    ((RRRN_T & 6) == 6 ? 0xffffff80 : 0));

        } else { /*BEQZ.Nn*/ /*BNEZ.Nn*/

            TCGCond eq_ne = (RRRN_T & 4) ? TCG_COND_NE : TCG_COND_EQ;



            gen_brcondi(dc, eq_ne, cpu_R[RRRN_S], 0,

                    4 + (RRRN_R | ((RRRN_T & 3) << 4)));

        }

        break;



    case 13: /*ST3n*/

        switch (RRRN_R) {

        case 0: /*MOV.Nn*/

            tcg_gen_mov_i32(cpu_R[RRRN_T], cpu_R[RRRN_S]);

            break;



        case 15: /*S3*/

            switch (RRRN_T) {

            case 0: /*RET.Nn*/

                gen_jump(dc, cpu_R[0]);

                break;



            case 1: /*RETW.Nn*/

                break;



            case 2: /*BREAK.Nn*/

                break;



            case 3: /*NOP.Nn*/

                break;



            case 6: /*ILL.Nn*/

                break;



            default: /*reserved*/

                break;

            }

            break;



        default: /*reserved*/

            break;

        }

        break;



    default: /*reserved*/

        break;

    }



    dc->pc = dc->next_pc;

    return;



invalid_opcode:

    qemu_log("INVALID(pc = %08x)\n", dc->pc);

    dc->pc = dc->next_pc;

#undef HAS_OPTION

}
