static int disas_thumb2_insn(CPUARMState *env, DisasContext *s, uint16_t insn_hw1)

{

    uint32_t insn, imm, shift, offset;

    uint32_t rd, rn, rm, rs;

    TCGv_i32 tmp;

    TCGv_i32 tmp2;

    TCGv_i32 tmp3;

    TCGv_i32 addr;

    TCGv_i64 tmp64;

    int op;

    int shiftop;

    int conds;

    int logic_cc;



    if (!(arm_feature(env, ARM_FEATURE_THUMB2)

          || arm_feature (env, ARM_FEATURE_M))) {

        /* Thumb-1 cores may need to treat bl and blx as a pair of

           16-bit instructions to get correct prefetch abort behavior.  */

        insn = insn_hw1;

        if ((insn & (1 << 12)) == 0) {

            ARCH(5);

            /* Second half of blx.  */

            offset = ((insn & 0x7ff) << 1);

            tmp = load_reg(s, 14);

            tcg_gen_addi_i32(tmp, tmp, offset);

            tcg_gen_andi_i32(tmp, tmp, 0xfffffffc);



            tmp2 = tcg_temp_new_i32();

            tcg_gen_movi_i32(tmp2, s->pc | 1);

            store_reg(s, 14, tmp2);

            gen_bx(s, tmp);

            return 0;

        }

        if (insn & (1 << 11)) {

            /* Second half of bl.  */

            offset = ((insn & 0x7ff) << 1) | 1;

            tmp = load_reg(s, 14);

            tcg_gen_addi_i32(tmp, tmp, offset);



            tmp2 = tcg_temp_new_i32();

            tcg_gen_movi_i32(tmp2, s->pc | 1);

            store_reg(s, 14, tmp2);

            gen_bx(s, tmp);

            return 0;

        }

        if ((s->pc & ~TARGET_PAGE_MASK) == 0) {

            /* Instruction spans a page boundary.  Implement it as two

               16-bit instructions in case the second half causes an

               prefetch abort.  */

            offset = ((int32_t)insn << 21) >> 9;

            tcg_gen_movi_i32(cpu_R[14], s->pc + 2 + offset);

            return 0;

        }

        /* Fall through to 32-bit decode.  */

    }



    insn = arm_lduw_code(env, s->pc, s->bswap_code);

    s->pc += 2;

    insn |= (uint32_t)insn_hw1 << 16;



    if ((insn & 0xf800e800) != 0xf000e800) {

        ARCH(6T2);

    }



    rn = (insn >> 16) & 0xf;

    rs = (insn >> 12) & 0xf;

    rd = (insn >> 8) & 0xf;

    rm = insn & 0xf;

    switch ((insn >> 25) & 0xf) {

    case 0: case 1: case 2: case 3:

        /* 16-bit instructions.  Should never happen.  */

        abort();

    case 4:

        if (insn & (1 << 22)) {

            /* Other load/store, table branch.  */

            if (insn & 0x01200000) {

                /* Load/store doubleword.  */

                if (rn == 15) {

                    addr = tcg_temp_new_i32();

                    tcg_gen_movi_i32(addr, s->pc & ~3);

                } else {

                    addr = load_reg(s, rn);

                }

                offset = (insn & 0xff) * 4;

                if ((insn & (1 << 23)) == 0)

                    offset = -offset;

                if (insn & (1 << 24)) {

                    tcg_gen_addi_i32(addr, addr, offset);

                    offset = 0;

                }

                if (insn & (1 << 20)) {

                    /* ldrd */

                    tmp = tcg_temp_new_i32();

                    tcg_gen_qemu_ld32u(tmp, addr, IS_USER(s));

                    store_reg(s, rs, tmp);

                    tcg_gen_addi_i32(addr, addr, 4);

                    tmp = tcg_temp_new_i32();

                    tcg_gen_qemu_ld32u(tmp, addr, IS_USER(s));

                    store_reg(s, rd, tmp);

                } else {

                    /* strd */

                    tmp = load_reg(s, rs);

                    tcg_gen_qemu_st32(tmp, addr, IS_USER(s));

                    tcg_temp_free_i32(tmp);

                    tcg_gen_addi_i32(addr, addr, 4);

                    tmp = load_reg(s, rd);

                    tcg_gen_qemu_st32(tmp, addr, IS_USER(s));

                    tcg_temp_free_i32(tmp);

                }

                if (insn & (1 << 21)) {

                    /* Base writeback.  */

                    if (rn == 15)

                        goto illegal_op;

                    tcg_gen_addi_i32(addr, addr, offset - 4);

                    store_reg(s, rn, addr);

                } else {

                    tcg_temp_free_i32(addr);

                }

            } else if ((insn & (1 << 23)) == 0) {

                /* Load/store exclusive word.  */

                addr = tcg_temp_local_new_i32();

                load_reg_var(s, addr, rn);

                tcg_gen_addi_i32(addr, addr, (insn & 0xff) << 2);

                if (insn & (1 << 20)) {

                    gen_load_exclusive(s, rs, 15, addr, 2);

                } else {

                    gen_store_exclusive(s, rd, rs, 15, addr, 2);

                }

                tcg_temp_free_i32(addr);

            } else if ((insn & (7 << 5)) == 0) {

                /* Table Branch.  */

                if (rn == 15) {

                    addr = tcg_temp_new_i32();

                    tcg_gen_movi_i32(addr, s->pc);

                } else {

                    addr = load_reg(s, rn);

                }

                tmp = load_reg(s, rm);

                tcg_gen_add_i32(addr, addr, tmp);

                if (insn & (1 << 4)) {

                    /* tbh */

                    tcg_gen_add_i32(addr, addr, tmp);

                    tcg_temp_free_i32(tmp);

                    tmp = tcg_temp_new_i32();

                    tcg_gen_qemu_ld16u(tmp, addr, IS_USER(s));

                } else { /* tbb */

                    tcg_temp_free_i32(tmp);

                    tmp = tcg_temp_new_i32();

                    tcg_gen_qemu_ld8u(tmp, addr, IS_USER(s));

                }

                tcg_temp_free_i32(addr);

                tcg_gen_shli_i32(tmp, tmp, 1);

                tcg_gen_addi_i32(tmp, tmp, s->pc);

                store_reg(s, 15, tmp);

            } else {

                int op2 = (insn >> 6) & 0x3;

                op = (insn >> 4) & 0x3;

                switch (op2) {

                case 0:

                    goto illegal_op;

                case 1:

                    /* Load/store exclusive byte/halfword/doubleword */

                    if (op == 2) {

                        goto illegal_op;

                    }

                    ARCH(7);

                    break;

                case 2:

                    /* Load-acquire/store-release */

                    if (op == 3) {

                        goto illegal_op;

                    }

                    /* Fall through */

                case 3:

                    /* Load-acquire/store-release exclusive */

                    ARCH(8);

                    break;

                }

                addr = tcg_temp_local_new_i32();

                load_reg_var(s, addr, rn);

                if (!(op2 & 1)) {

                    if (insn & (1 << 20)) {

                        tmp = tcg_temp_new_i32();

                        switch (op) {

                        case 0: /* ldab */

                            tcg_gen_qemu_ld8u(tmp, addr, IS_USER(s));

                            break;

                        case 1: /* ldah */

                            tcg_gen_qemu_ld16u(tmp, addr, IS_USER(s));

                            break;

                        case 2: /* lda */

                            tcg_gen_qemu_ld32u(tmp, addr, IS_USER(s));

                            break;

                        default:

                            abort();

                        }

                        store_reg(s, rs, tmp);

                    } else {

                        tmp = load_reg(s, rs);

                        switch (op) {

                        case 0: /* stlb */

                            tcg_gen_qemu_st8(tmp, addr, IS_USER(s));

                            break;

                        case 1: /* stlh */

                            tcg_gen_qemu_st16(tmp, addr, IS_USER(s));

                            break;

                        case 2: /* stl */

                            tcg_gen_qemu_st32(tmp, addr, IS_USER(s));

                            break;

                        default:

                            abort();

                        }

                        tcg_temp_free_i32(tmp);

                    }

                } else if (insn & (1 << 20)) {

                    gen_load_exclusive(s, rs, rd, addr, op);

                } else {

                    gen_store_exclusive(s, rm, rs, rd, addr, op);

                }

                tcg_temp_free_i32(addr);

            }

        } else {

            /* Load/store multiple, RFE, SRS.  */

            if (((insn >> 23) & 1) == ((insn >> 24) & 1)) {

                /* RFE, SRS: not available in user mode or on M profile */

                if (IS_USER(s) || IS_M(env)) {

                    goto illegal_op;

                }

                if (insn & (1 << 20)) {

                    /* rfe */

                    addr = load_reg(s, rn);

                    if ((insn & (1 << 24)) == 0)

                        tcg_gen_addi_i32(addr, addr, -8);

                    /* Load PC into tmp and CPSR into tmp2.  */

                    tmp = tcg_temp_new_i32();

                    tcg_gen_qemu_ld32u(tmp, addr, 0);

                    tcg_gen_addi_i32(addr, addr, 4);

                    tmp2 = tcg_temp_new_i32();

                    tcg_gen_qemu_ld32u(tmp2, addr, 0);

                    if (insn & (1 << 21)) {

                        /* Base writeback.  */

                        if (insn & (1 << 24)) {

                            tcg_gen_addi_i32(addr, addr, 4);

                        } else {

                            tcg_gen_addi_i32(addr, addr, -4);

                        }

                        store_reg(s, rn, addr);

                    } else {

                        tcg_temp_free_i32(addr);

                    }

                    gen_rfe(s, tmp, tmp2);

                } else {

                    /* srs */

                    gen_srs(s, (insn & 0x1f), (insn & (1 << 24)) ? 1 : 2,

                            insn & (1 << 21));

                }

            } else {

                int i, loaded_base = 0;

                TCGv_i32 loaded_var;

                /* Load/store multiple.  */

                addr = load_reg(s, rn);

                offset = 0;

                for (i = 0; i < 16; i++) {

                    if (insn & (1 << i))

                        offset += 4;

                }

                if (insn & (1 << 24)) {

                    tcg_gen_addi_i32(addr, addr, -offset);

                }



                TCGV_UNUSED_I32(loaded_var);

                for (i = 0; i < 16; i++) {

                    if ((insn & (1 << i)) == 0)

                        continue;

                    if (insn & (1 << 20)) {

                        /* Load.  */

                        tmp = tcg_temp_new_i32();

                        tcg_gen_qemu_ld32u(tmp, addr, IS_USER(s));

                        if (i == 15) {

                            gen_bx(s, tmp);

                        } else if (i == rn) {

                            loaded_var = tmp;

                            loaded_base = 1;

                        } else {

                            store_reg(s, i, tmp);

                        }

                    } else {

                        /* Store.  */

                        tmp = load_reg(s, i);

                        tcg_gen_qemu_st32(tmp, addr, IS_USER(s));

                        tcg_temp_free_i32(tmp);

                    }

                    tcg_gen_addi_i32(addr, addr, 4);

                }

                if (loaded_base) {

                    store_reg(s, rn, loaded_var);

                }

                if (insn & (1 << 21)) {

                    /* Base register writeback.  */

                    if (insn & (1 << 24)) {

                        tcg_gen_addi_i32(addr, addr, -offset);

                    }

                    /* Fault if writeback register is in register list.  */

                    if (insn & (1 << rn))

                        goto illegal_op;

                    store_reg(s, rn, addr);

                } else {

                    tcg_temp_free_i32(addr);

                }

            }

        }

        break;

    case 5:



        op = (insn >> 21) & 0xf;

        if (op == 6) {

            /* Halfword pack.  */

            tmp = load_reg(s, rn);

            tmp2 = load_reg(s, rm);

            shift = ((insn >> 10) & 0x1c) | ((insn >> 6) & 0x3);

            if (insn & (1 << 5)) {

                /* pkhtb */

                if (shift == 0)

                    shift = 31;

                tcg_gen_sari_i32(tmp2, tmp2, shift);

                tcg_gen_andi_i32(tmp, tmp, 0xffff0000);

                tcg_gen_ext16u_i32(tmp2, tmp2);

            } else {

                /* pkhbt */

                if (shift)

                    tcg_gen_shli_i32(tmp2, tmp2, shift);

                tcg_gen_ext16u_i32(tmp, tmp);

                tcg_gen_andi_i32(tmp2, tmp2, 0xffff0000);

            }

            tcg_gen_or_i32(tmp, tmp, tmp2);

            tcg_temp_free_i32(tmp2);

            store_reg(s, rd, tmp);

        } else {

            /* Data processing register constant shift.  */

            if (rn == 15) {

                tmp = tcg_temp_new_i32();

                tcg_gen_movi_i32(tmp, 0);

            } else {

                tmp = load_reg(s, rn);

            }

            tmp2 = load_reg(s, rm);



            shiftop = (insn >> 4) & 3;

            shift = ((insn >> 6) & 3) | ((insn >> 10) & 0x1c);

            conds = (insn & (1 << 20)) != 0;

            logic_cc = (conds && thumb2_logic_op(op));

            gen_arm_shift_im(tmp2, shiftop, shift, logic_cc);

            if (gen_thumb2_data_op(s, op, conds, 0, tmp, tmp2))

                goto illegal_op;

            tcg_temp_free_i32(tmp2);

            if (rd != 15) {

                store_reg(s, rd, tmp);

            } else {

                tcg_temp_free_i32(tmp);

            }

        }

        break;

    case 13: /* Misc data processing.  */

        op = ((insn >> 22) & 6) | ((insn >> 7) & 1);

        if (op < 4 && (insn & 0xf000) != 0xf000)

            goto illegal_op;

        switch (op) {

        case 0: /* Register controlled shift.  */

            tmp = load_reg(s, rn);

            tmp2 = load_reg(s, rm);

            if ((insn & 0x70) != 0)

                goto illegal_op;

            op = (insn >> 21) & 3;

            logic_cc = (insn & (1 << 20)) != 0;

            gen_arm_shift_reg(tmp, op, tmp2, logic_cc);

            if (logic_cc)

                gen_logic_CC(tmp);

            store_reg_bx(env, s, rd, tmp);

            break;

        case 1: /* Sign/zero extend.  */

            tmp = load_reg(s, rm);

            shift = (insn >> 4) & 3;

            /* ??? In many cases it's not necessary to do a

               rotate, a shift is sufficient.  */

            if (shift != 0)

                tcg_gen_rotri_i32(tmp, tmp, shift * 8);

            op = (insn >> 20) & 7;

            switch (op) {

            case 0: gen_sxth(tmp);   break;

            case 1: gen_uxth(tmp);   break;

            case 2: gen_sxtb16(tmp); break;

            case 3: gen_uxtb16(tmp); break;

            case 4: gen_sxtb(tmp);   break;

            case 5: gen_uxtb(tmp);   break;

            default: goto illegal_op;

            }

            if (rn != 15) {

                tmp2 = load_reg(s, rn);

                if ((op >> 1) == 1) {

                    gen_add16(tmp, tmp2);

                } else {

                    tcg_gen_add_i32(tmp, tmp, tmp2);

                    tcg_temp_free_i32(tmp2);

                }

            }

            store_reg(s, rd, tmp);

            break;

        case 2: /* SIMD add/subtract.  */

            op = (insn >> 20) & 7;

            shift = (insn >> 4) & 7;

            if ((op & 3) == 3 || (shift & 3) == 3)

                goto illegal_op;

            tmp = load_reg(s, rn);

            tmp2 = load_reg(s, rm);

            gen_thumb2_parallel_addsub(op, shift, tmp, tmp2);

            tcg_temp_free_i32(tmp2);

            store_reg(s, rd, tmp);

            break;

        case 3: /* Other data processing.  */

            op = ((insn >> 17) & 0x38) | ((insn >> 4) & 7);

            if (op < 4) {

                /* Saturating add/subtract.  */

                tmp = load_reg(s, rn);

                tmp2 = load_reg(s, rm);

                if (op & 1)

                    gen_helper_double_saturate(tmp, cpu_env, tmp);

                if (op & 2)

                    gen_helper_sub_saturate(tmp, cpu_env, tmp2, tmp);

                else

                    gen_helper_add_saturate(tmp, cpu_env, tmp, tmp2);

                tcg_temp_free_i32(tmp2);

            } else {

                tmp = load_reg(s, rn);

                switch (op) {

                case 0x0a: /* rbit */

                    gen_helper_rbit(tmp, tmp);

                    break;

                case 0x08: /* rev */

                    tcg_gen_bswap32_i32(tmp, tmp);

                    break;

                case 0x09: /* rev16 */

                    gen_rev16(tmp);

                    break;

                case 0x0b: /* revsh */

                    gen_revsh(tmp);

                    break;

                case 0x10: /* sel */

                    tmp2 = load_reg(s, rm);

                    tmp3 = tcg_temp_new_i32();

                    tcg_gen_ld_i32(tmp3, cpu_env, offsetof(CPUARMState, GE));

                    gen_helper_sel_flags(tmp, tmp3, tmp, tmp2);

                    tcg_temp_free_i32(tmp3);

                    tcg_temp_free_i32(tmp2);

                    break;

                case 0x18: /* clz */

                    gen_helper_clz(tmp, tmp);

                    break;

                default:

                    goto illegal_op;

                }

            }

            store_reg(s, rd, tmp);

            break;

        case 4: case 5: /* 32-bit multiply.  Sum of absolute differences.  */

            op = (insn >> 4) & 0xf;

            tmp = load_reg(s, rn);

            tmp2 = load_reg(s, rm);

            switch ((insn >> 20) & 7) {

            case 0: /* 32 x 32 -> 32 */

                tcg_gen_mul_i32(tmp, tmp, tmp2);

                tcg_temp_free_i32(tmp2);

                if (rs != 15) {

                    tmp2 = load_reg(s, rs);

                    if (op)

                        tcg_gen_sub_i32(tmp, tmp2, tmp);

                    else

                        tcg_gen_add_i32(tmp, tmp, tmp2);

                    tcg_temp_free_i32(tmp2);

                }

                break;

            case 1: /* 16 x 16 -> 32 */

                gen_mulxy(tmp, tmp2, op & 2, op & 1);

                tcg_temp_free_i32(tmp2);

                if (rs != 15) {

                    tmp2 = load_reg(s, rs);

                    gen_helper_add_setq(tmp, cpu_env, tmp, tmp2);

                    tcg_temp_free_i32(tmp2);

                }

                break;

            case 2: /* Dual multiply add.  */

            case 4: /* Dual multiply subtract.  */

                if (op)

                    gen_swap_half(tmp2);

                gen_smul_dual(tmp, tmp2);

                if (insn & (1 << 22)) {

                    /* This subtraction cannot overflow. */

                    tcg_gen_sub_i32(tmp, tmp, tmp2);

                } else {

                    /* This addition cannot overflow 32 bits;

                     * however it may overflow considered as a signed

                     * operation, in which case we must set the Q flag.

                     */

                    gen_helper_add_setq(tmp, cpu_env, tmp, tmp2);

                }

                tcg_temp_free_i32(tmp2);

                if (rs != 15)

                  {

                    tmp2 = load_reg(s, rs);

                    gen_helper_add_setq(tmp, cpu_env, tmp, tmp2);

                    tcg_temp_free_i32(tmp2);

                  }

                break;

            case 3: /* 32 * 16 -> 32msb */

                if (op)

                    tcg_gen_sari_i32(tmp2, tmp2, 16);

                else

                    gen_sxth(tmp2);

                tmp64 = gen_muls_i64_i32(tmp, tmp2);

                tcg_gen_shri_i64(tmp64, tmp64, 16);

                tmp = tcg_temp_new_i32();

                tcg_gen_trunc_i64_i32(tmp, tmp64);

                tcg_temp_free_i64(tmp64);

                if (rs != 15)

                  {

                    tmp2 = load_reg(s, rs);

                    gen_helper_add_setq(tmp, cpu_env, tmp, tmp2);

                    tcg_temp_free_i32(tmp2);

                  }

                break;

            case 5: case 6: /* 32 * 32 -> 32msb (SMMUL, SMMLA, SMMLS) */

                tmp64 = gen_muls_i64_i32(tmp, tmp2);

                if (rs != 15) {

                    tmp = load_reg(s, rs);

                    if (insn & (1 << 20)) {

                        tmp64 = gen_addq_msw(tmp64, tmp);

                    } else {

                        tmp64 = gen_subq_msw(tmp64, tmp);

                    }

                }

                if (insn & (1 << 4)) {

                    tcg_gen_addi_i64(tmp64, tmp64, 0x80000000u);

                }

                tcg_gen_shri_i64(tmp64, tmp64, 32);

                tmp = tcg_temp_new_i32();

                tcg_gen_trunc_i64_i32(tmp, tmp64);

                tcg_temp_free_i64(tmp64);

                break;

            case 7: /* Unsigned sum of absolute differences.  */

                gen_helper_usad8(tmp, tmp, tmp2);

                tcg_temp_free_i32(tmp2);

                if (rs != 15) {

                    tmp2 = load_reg(s, rs);

                    tcg_gen_add_i32(tmp, tmp, tmp2);

                    tcg_temp_free_i32(tmp2);

                }

                break;

            }

            store_reg(s, rd, tmp);

            break;

        case 6: case 7: /* 64-bit multiply, Divide.  */

            op = ((insn >> 4) & 0xf) | ((insn >> 16) & 0x70);

            tmp = load_reg(s, rn);

            tmp2 = load_reg(s, rm);

            if ((op & 0x50) == 0x10) {

                /* sdiv, udiv */

                if (!arm_feature(env, ARM_FEATURE_THUMB_DIV)) {

                    goto illegal_op;

                }

                if (op & 0x20)

                    gen_helper_udiv(tmp, tmp, tmp2);

                else

                    gen_helper_sdiv(tmp, tmp, tmp2);

                tcg_temp_free_i32(tmp2);

                store_reg(s, rd, tmp);

            } else if ((op & 0xe) == 0xc) {

                /* Dual multiply accumulate long.  */

                if (op & 1)

                    gen_swap_half(tmp2);

                gen_smul_dual(tmp, tmp2);

                if (op & 0x10) {

                    tcg_gen_sub_i32(tmp, tmp, tmp2);

                } else {

                    tcg_gen_add_i32(tmp, tmp, tmp2);

                }

                tcg_temp_free_i32(tmp2);

                /* BUGFIX */

                tmp64 = tcg_temp_new_i64();

                tcg_gen_ext_i32_i64(tmp64, tmp);

                tcg_temp_free_i32(tmp);

                gen_addq(s, tmp64, rs, rd);

                gen_storeq_reg(s, rs, rd, tmp64);

                tcg_temp_free_i64(tmp64);

            } else {

                if (op & 0x20) {

                    /* Unsigned 64-bit multiply  */

                    tmp64 = gen_mulu_i64_i32(tmp, tmp2);

                } else {

                    if (op & 8) {

                        /* smlalxy */

                        gen_mulxy(tmp, tmp2, op & 2, op & 1);

                        tcg_temp_free_i32(tmp2);

                        tmp64 = tcg_temp_new_i64();

                        tcg_gen_ext_i32_i64(tmp64, tmp);

                        tcg_temp_free_i32(tmp);

                    } else {

                        /* Signed 64-bit multiply  */

                        tmp64 = gen_muls_i64_i32(tmp, tmp2);

                    }

                }

                if (op & 4) {

                    /* umaal */

                    gen_addq_lo(s, tmp64, rs);

                    gen_addq_lo(s, tmp64, rd);

                } else if (op & 0x40) {

                    /* 64-bit accumulate.  */

                    gen_addq(s, tmp64, rs, rd);

                }

                gen_storeq_reg(s, rs, rd, tmp64);

                tcg_temp_free_i64(tmp64);

            }

            break;

        }

        break;

    case 6: case 7: case 14: case 15:

        /* Coprocessor.  */

        if (((insn >> 24) & 3) == 3) {

            /* Translate into the equivalent ARM encoding.  */

            insn = (insn & 0xe2ffffff) | ((insn & (1 << 28)) >> 4) | (1 << 28);

            if (disas_neon_data_insn(env, s, insn))

                goto illegal_op;

        } else {

            if (insn & (1 << 28))

                goto illegal_op;

            if (disas_coproc_insn (env, s, insn))

                goto illegal_op;

        }

        break;

    case 8: case 9: case 10: case 11:

        if (insn & (1 << 15)) {

            /* Branches, misc control.  */

            if (insn & 0x5000) {

                /* Unconditional branch.  */

                /* signextend(hw1[10:0]) -> offset[:12].  */

                offset = ((int32_t)insn << 5) >> 9 & ~(int32_t)0xfff;

                /* hw1[10:0] -> offset[11:1].  */

                offset |= (insn & 0x7ff) << 1;

                /* (~hw2[13, 11] ^ offset[24]) -> offset[23,22]

                   offset[24:22] already have the same value because of the

                   sign extension above.  */

                offset ^= ((~insn) & (1 << 13)) << 10;

                offset ^= ((~insn) & (1 << 11)) << 11;



                if (insn & (1 << 14)) {

                    /* Branch and link.  */

                    tcg_gen_movi_i32(cpu_R[14], s->pc | 1);

                }



                offset += s->pc;

                if (insn & (1 << 12)) {

                    /* b/bl */

                    gen_jmp(s, offset);

                } else {

                    /* blx */

                    offset &= ~(uint32_t)2;

                    /* thumb2 bx, no need to check */

                    gen_bx_im(s, offset);

                }

            } else if (((insn >> 23) & 7) == 7) {

                /* Misc control */

                if (insn & (1 << 13))

                    goto illegal_op;



                if (insn & (1 << 26)) {

                    /* Secure monitor call (v6Z) */



                    goto illegal_op; /* not implemented.  */

                } else {

                    op = (insn >> 20) & 7;

                    switch (op) {

                    case 0: /* msr cpsr.  */

                        if (IS_M(env)) {

                            tmp = load_reg(s, rn);

                            addr = tcg_const_i32(insn & 0xff);

                            gen_helper_v7m_msr(cpu_env, addr, tmp);

                            tcg_temp_free_i32(addr);

                            tcg_temp_free_i32(tmp);

                            gen_lookup_tb(s);

                            break;

                        }

                        /* fall through */

                    case 1: /* msr spsr.  */

                        if (IS_M(env))

                            goto illegal_op;

                        tmp = load_reg(s, rn);

                        if (gen_set_psr(s,

                              msr_mask(env, s, (insn >> 8) & 0xf, op == 1),

                              op == 1, tmp))

                            goto illegal_op;

                        break;

                    case 2: /* cps, nop-hint.  */

                        if (((insn >> 8) & 7) == 0) {

                            gen_nop_hint(s, insn & 0xff);

                        }

                        /* Implemented as NOP in user mode.  */

                        if (IS_USER(s))

                            break;

                        offset = 0;

                        imm = 0;

                        if (insn & (1 << 10)) {

                            if (insn & (1 << 7))

                                offset |= CPSR_A;

                            if (insn & (1 << 6))

                                offset |= CPSR_I;

                            if (insn & (1 << 5))

                                offset |= CPSR_F;

                            if (insn & (1 << 9))

                                imm = CPSR_A | CPSR_I | CPSR_F;

                        }

                        if (insn & (1 << 8)) {

                            offset |= 0x1f;

                            imm |= (insn & 0x1f);

                        }

                        if (offset) {

                            gen_set_psr_im(s, offset, 0, imm);

                        }

                        break;

                    case 3: /* Special control operations.  */

                        ARCH(7);

                        op = (insn >> 4) & 0xf;

                        switch (op) {

                        case 2: /* clrex */

                            gen_clrex(s);

                            break;

                        case 4: /* dsb */

                        case 5: /* dmb */

                        case 6: /* isb */

                            /* These execute as NOPs.  */

                            break;

                        default:

                            goto illegal_op;

                        }

                        break;

                    case 4: /* bxj */

                        /* Trivial implementation equivalent to bx.  */

                        tmp = load_reg(s, rn);

                        gen_bx(s, tmp);

                        break;

                    case 5: /* Exception return.  */

                        if (IS_USER(s)) {

                            goto illegal_op;

                        }

                        if (rn != 14 || rd != 15) {

                            goto illegal_op;

                        }

                        tmp = load_reg(s, rn);

                        tcg_gen_subi_i32(tmp, tmp, insn & 0xff);

                        gen_exception_return(s, tmp);

                        break;

                    case 6: /* mrs cpsr.  */

                        tmp = tcg_temp_new_i32();

                        if (IS_M(env)) {

                            addr = tcg_const_i32(insn & 0xff);

                            gen_helper_v7m_mrs(tmp, cpu_env, addr);

                            tcg_temp_free_i32(addr);

                        } else {

                            gen_helper_cpsr_read(tmp, cpu_env);

                        }

                        store_reg(s, rd, tmp);

                        break;

                    case 7: /* mrs spsr.  */

                        /* Not accessible in user mode.  */

                        if (IS_USER(s) || IS_M(env))

                            goto illegal_op;

                        tmp = load_cpu_field(spsr);

                        store_reg(s, rd, tmp);

                        break;

                    }

                }

            } else {

                /* Conditional branch.  */

                op = (insn >> 22) & 0xf;

                /* Generate a conditional jump to next instruction.  */

                s->condlabel = gen_new_label();

                gen_test_cc(op ^ 1, s->condlabel);

                s->condjmp = 1;



                /* offset[11:1] = insn[10:0] */

                offset = (insn & 0x7ff) << 1;

                /* offset[17:12] = insn[21:16].  */

                offset |= (insn & 0x003f0000) >> 4;

                /* offset[31:20] = insn[26].  */

                offset |= ((int32_t)((insn << 5) & 0x80000000)) >> 11;

                /* offset[18] = insn[13].  */

                offset |= (insn & (1 << 13)) << 5;

                /* offset[19] = insn[11].  */

                offset |= (insn & (1 << 11)) << 8;



                /* jump to the offset */

                gen_jmp(s, s->pc + offset);

            }

        } else {

            /* Data processing immediate.  */

            if (insn & (1 << 25)) {

                if (insn & (1 << 24)) {

                    if (insn & (1 << 20))

                        goto illegal_op;

                    /* Bitfield/Saturate.  */

                    op = (insn >> 21) & 7;

                    imm = insn & 0x1f;

                    shift = ((insn >> 6) & 3) | ((insn >> 10) & 0x1c);

                    if (rn == 15) {

                        tmp = tcg_temp_new_i32();

                        tcg_gen_movi_i32(tmp, 0);

                    } else {

                        tmp = load_reg(s, rn);

                    }

                    switch (op) {

                    case 2: /* Signed bitfield extract.  */

                        imm++;

                        if (shift + imm > 32)

                            goto illegal_op;

                        if (imm < 32)

                            gen_sbfx(tmp, shift, imm);

                        break;

                    case 6: /* Unsigned bitfield extract.  */

                        imm++;

                        if (shift + imm > 32)

                            goto illegal_op;

                        if (imm < 32)

                            gen_ubfx(tmp, shift, (1u << imm) - 1);

                        break;

                    case 3: /* Bitfield insert/clear.  */

                        if (imm < shift)

                            goto illegal_op;

                        imm = imm + 1 - shift;

                        if (imm != 32) {

                            tmp2 = load_reg(s, rd);

                            tcg_gen_deposit_i32(tmp, tmp2, tmp, shift, imm);

                            tcg_temp_free_i32(tmp2);

                        }

                        break;

                    case 7:

                        goto illegal_op;

                    default: /* Saturate.  */

                        if (shift) {

                            if (op & 1)

                                tcg_gen_sari_i32(tmp, tmp, shift);

                            else

                                tcg_gen_shli_i32(tmp, tmp, shift);

                        }

                        tmp2 = tcg_const_i32(imm);

                        if (op & 4) {

                            /* Unsigned.  */

                            if ((op & 1) && shift == 0)

                                gen_helper_usat16(tmp, cpu_env, tmp, tmp2);

                            else

                                gen_helper_usat(tmp, cpu_env, tmp, tmp2);

                        } else {

                            /* Signed.  */

                            if ((op & 1) && shift == 0)

                                gen_helper_ssat16(tmp, cpu_env, tmp, tmp2);

                            else

                                gen_helper_ssat(tmp, cpu_env, tmp, tmp2);

                        }

                        tcg_temp_free_i32(tmp2);

                        break;

                    }

                    store_reg(s, rd, tmp);

                } else {

                    imm = ((insn & 0x04000000) >> 15)

                          | ((insn & 0x7000) >> 4) | (insn & 0xff);

                    if (insn & (1 << 22)) {

                        /* 16-bit immediate.  */

                        imm |= (insn >> 4) & 0xf000;

                        if (insn & (1 << 23)) {

                            /* movt */

                            tmp = load_reg(s, rd);

                            tcg_gen_ext16u_i32(tmp, tmp);

                            tcg_gen_ori_i32(tmp, tmp, imm << 16);

                        } else {

                            /* movw */

                            tmp = tcg_temp_new_i32();

                            tcg_gen_movi_i32(tmp, imm);

                        }

                    } else {

                        /* Add/sub 12-bit immediate.  */

                        if (rn == 15) {

                            offset = s->pc & ~(uint32_t)3;

                            if (insn & (1 << 23))

                                offset -= imm;

                            else

                                offset += imm;

                            tmp = tcg_temp_new_i32();

                            tcg_gen_movi_i32(tmp, offset);

                        } else {

                            tmp = load_reg(s, rn);

                            if (insn & (1 << 23))

                                tcg_gen_subi_i32(tmp, tmp, imm);

                            else

                                tcg_gen_addi_i32(tmp, tmp, imm);

                        }

                    }

                    store_reg(s, rd, tmp);

                }

            } else {

                int shifter_out = 0;

                /* modified 12-bit immediate.  */

                shift = ((insn & 0x04000000) >> 23) | ((insn & 0x7000) >> 12);

                imm = (insn & 0xff);

                switch (shift) {

                case 0: /* XY */

                    /* Nothing to do.  */

                    break;

                case 1: /* 00XY00XY */

                    imm |= imm << 16;

                    break;

                case 2: /* XY00XY00 */

                    imm |= imm << 16;

                    imm <<= 8;

                    break;

                case 3: /* XYXYXYXY */

                    imm |= imm << 16;

                    imm |= imm << 8;

                    break;

                default: /* Rotated constant.  */

                    shift = (shift << 1) | (imm >> 7);

                    imm |= 0x80;

                    imm = imm << (32 - shift);

                    shifter_out = 1;

                    break;

                }

                tmp2 = tcg_temp_new_i32();

                tcg_gen_movi_i32(tmp2, imm);

                rn = (insn >> 16) & 0xf;

                if (rn == 15) {

                    tmp = tcg_temp_new_i32();

                    tcg_gen_movi_i32(tmp, 0);

                } else {

                    tmp = load_reg(s, rn);

                }

                op = (insn >> 21) & 0xf;

                if (gen_thumb2_data_op(s, op, (insn & (1 << 20)) != 0,

                                       shifter_out, tmp, tmp2))

                    goto illegal_op;

                tcg_temp_free_i32(tmp2);

                rd = (insn >> 8) & 0xf;

                if (rd != 15) {

                    store_reg(s, rd, tmp);

                } else {

                    tcg_temp_free_i32(tmp);

                }

            }

        }

        break;

    case 12: /* Load/store single data item.  */

        {

        int postinc = 0;

        int writeback = 0;

        int user;

        if ((insn & 0x01100000) == 0x01000000) {

            if (disas_neon_ls_insn(env, s, insn))

                goto illegal_op;

            break;

        }

        op = ((insn >> 21) & 3) | ((insn >> 22) & 4);

        if (rs == 15) {

            if (!(insn & (1 << 20))) {

                goto illegal_op;

            }

            if (op != 2) {

                /* Byte or halfword load space with dest == r15 : memory hints.

                 * Catch them early so we don't emit pointless addressing code.

                 * This space is a mix of:

                 *  PLD/PLDW/PLI,  which we implement as NOPs (note that unlike

                 *     the ARM encodings, PLDW space doesn't UNDEF for non-v7MP

                 *     cores)

                 *  unallocated hints, which must be treated as NOPs

                 *  UNPREDICTABLE space, which we NOP or UNDEF depending on

                 *     which is easiest for the decoding logic

                 *  Some space which must UNDEF

                 */

                int op1 = (insn >> 23) & 3;

                int op2 = (insn >> 6) & 0x3f;

                if (op & 2) {

                    goto illegal_op;

                }

                if (rn == 15) {

                    /* UNPREDICTABLE, unallocated hint or

                     * PLD/PLDW/PLI (literal)

                     */

                    return 0;

                }

                if (op1 & 1) {

                    return 0; /* PLD/PLDW/PLI or unallocated hint */

                }

                if ((op2 == 0) || ((op2 & 0x3c) == 0x30)) {

                    return 0; /* PLD/PLDW/PLI or unallocated hint */

                }

                /* UNDEF space, or an UNPREDICTABLE */

                return 1;

            }

        }

        user = IS_USER(s);

        if (rn == 15) {

            addr = tcg_temp_new_i32();

            /* PC relative.  */

            /* s->pc has already been incremented by 4.  */

            imm = s->pc & 0xfffffffc;

            if (insn & (1 << 23))

                imm += insn & 0xfff;

            else

                imm -= insn & 0xfff;

            tcg_gen_movi_i32(addr, imm);

        } else {

            addr = load_reg(s, rn);

            if (insn & (1 << 23)) {

                /* Positive offset.  */

                imm = insn & 0xfff;

                tcg_gen_addi_i32(addr, addr, imm);

            } else {

                imm = insn & 0xff;

                switch ((insn >> 8) & 0xf) {

                case 0x0: /* Shifted Register.  */

                    shift = (insn >> 4) & 0xf;

                    if (shift > 3) {

                        tcg_temp_free_i32(addr);

                        goto illegal_op;

                    }

                    tmp = load_reg(s, rm);

                    if (shift)

                        tcg_gen_shli_i32(tmp, tmp, shift);

                    tcg_gen_add_i32(addr, addr, tmp);

                    tcg_temp_free_i32(tmp);

                    break;

                case 0xc: /* Negative offset.  */

                    tcg_gen_addi_i32(addr, addr, -imm);

                    break;

                case 0xe: /* User privilege.  */

                    tcg_gen_addi_i32(addr, addr, imm);

                    user = 1;

                    break;

                case 0x9: /* Post-decrement.  */

                    imm = -imm;

                    /* Fall through.  */

                case 0xb: /* Post-increment.  */

                    postinc = 1;

                    writeback = 1;

                    break;

                case 0xd: /* Pre-decrement.  */

                    imm = -imm;

                    /* Fall through.  */

                case 0xf: /* Pre-increment.  */

                    tcg_gen_addi_i32(addr, addr, imm);

                    writeback = 1;

                    break;

                default:

                    tcg_temp_free_i32(addr);

                    goto illegal_op;

                }

            }

        }

        if (insn & (1 << 20)) {

            /* Load.  */

            tmp = tcg_temp_new_i32();

            switch (op) {

            case 0:

                tcg_gen_qemu_ld8u(tmp, addr, user);

                break;

            case 4:

                tcg_gen_qemu_ld8s(tmp, addr, user);

                break;

            case 1:

                tcg_gen_qemu_ld16u(tmp, addr, user);

                break;

            case 5:

                tcg_gen_qemu_ld16s(tmp, addr, user);

                break;

            case 2:

                tcg_gen_qemu_ld32u(tmp, addr, user);

                break;

            default:

                tcg_temp_free_i32(tmp);

                tcg_temp_free_i32(addr);

                goto illegal_op;

            }

            if (rs == 15) {

                gen_bx(s, tmp);

            } else {

                store_reg(s, rs, tmp);

            }

        } else {

            /* Store.  */

            tmp = load_reg(s, rs);

            switch (op) {

            case 0:

                tcg_gen_qemu_st8(tmp, addr, user);

                break;

            case 1:

                tcg_gen_qemu_st16(tmp, addr, user);

                break;

            case 2:

                tcg_gen_qemu_st32(tmp, addr, user);

                break;

            default:

                tcg_temp_free_i32(tmp);

                tcg_temp_free_i32(addr);

                goto illegal_op;

            }

            tcg_temp_free_i32(tmp);

        }

        if (postinc)

            tcg_gen_addi_i32(addr, addr, imm);

        if (writeback) {

            store_reg(s, rn, addr);

        } else {

            tcg_temp_free_i32(addr);

        }

        }

        break;

    default:

        goto illegal_op;

    }

    return 0;

illegal_op:

    return 1;

}