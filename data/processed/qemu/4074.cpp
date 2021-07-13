static void disas_sparc_insn(DisasContext * dc, unsigned int insn)

{

    unsigned int opc, rs1, rs2, rd;

    TCGv cpu_src1, cpu_src2;

    TCGv_i32 cpu_src1_32, cpu_src2_32, cpu_dst_32;

    TCGv_i64 cpu_src1_64, cpu_src2_64, cpu_dst_64;

    target_long simm;



    if (unlikely(qemu_loglevel_mask(CPU_LOG_TB_OP | CPU_LOG_TB_OP_OPT))) {

        tcg_gen_debug_insn_start(dc->pc);

    }



    opc = GET_FIELD(insn, 0, 1);



    rd = GET_FIELD(insn, 2, 6);



    switch (opc) {

    case 0:                     /* branches/sethi */

        {

            unsigned int xop = GET_FIELD(insn, 7, 9);

            int32_t target;

            switch (xop) {

#ifdef TARGET_SPARC64

            case 0x1:           /* V9 BPcc */

                {

                    int cc;



                    target = GET_FIELD_SP(insn, 0, 18);

                    target = sign_extend(target, 19);

                    target <<= 2;

                    cc = GET_FIELD_SP(insn, 20, 21);

                    if (cc == 0)

                        do_branch(dc, target, insn, 0);

                    else if (cc == 2)

                        do_branch(dc, target, insn, 1);

                    else

                        goto illegal_insn;

                    goto jmp_insn;

                }

            case 0x3:           /* V9 BPr */

                {

                    target = GET_FIELD_SP(insn, 0, 13) |

                        (GET_FIELD_SP(insn, 20, 21) << 14);

                    target = sign_extend(target, 16);

                    target <<= 2;

                    cpu_src1 = get_src1(dc, insn);

                    do_branch_reg(dc, target, insn, cpu_src1);

                    goto jmp_insn;

                }

            case 0x5:           /* V9 FBPcc */

                {

                    int cc = GET_FIELD_SP(insn, 20, 21);

                    if (gen_trap_ifnofpu(dc)) {

                        goto jmp_insn;

                    }

                    target = GET_FIELD_SP(insn, 0, 18);

                    target = sign_extend(target, 19);

                    target <<= 2;

                    do_fbranch(dc, target, insn, cc);

                    goto jmp_insn;

                }

#else

            case 0x7:           /* CBN+x */

                {

                    goto ncp_insn;

                }

#endif

            case 0x2:           /* BN+x */

                {

                    target = GET_FIELD(insn, 10, 31);

                    target = sign_extend(target, 22);

                    target <<= 2;

                    do_branch(dc, target, insn, 0);

                    goto jmp_insn;

                }

            case 0x6:           /* FBN+x */

                {

                    if (gen_trap_ifnofpu(dc)) {

                        goto jmp_insn;

                    }

                    target = GET_FIELD(insn, 10, 31);

                    target = sign_extend(target, 22);

                    target <<= 2;

                    do_fbranch(dc, target, insn, 0);

                    goto jmp_insn;

                }

            case 0x4:           /* SETHI */

                /* Special-case %g0 because that's the canonical nop.  */

                if (rd) {

                    uint32_t value = GET_FIELD(insn, 10, 31);

                    TCGv t = gen_dest_gpr(dc, rd);

                    tcg_gen_movi_tl(t, value << 10);

                    gen_store_gpr(dc, rd, t);

                }

                break;

            case 0x0:           /* UNIMPL */

            default:

                goto illegal_insn;

            }

            break;

        }

        break;

    case 1:                     /*CALL*/

        {

            target_long target = GET_FIELDs(insn, 2, 31) << 2;

            TCGv o7 = gen_dest_gpr(dc, 15);



            tcg_gen_movi_tl(o7, dc->pc);

            gen_store_gpr(dc, 15, o7);

            target += dc->pc;

            gen_mov_pc_npc(dc);

#ifdef TARGET_SPARC64

            if (unlikely(AM_CHECK(dc))) {

                target &= 0xffffffffULL;

            }

#endif

            dc->npc = target;

        }

        goto jmp_insn;

    case 2:                     /* FPU & Logical Operations */

        {

            unsigned int xop = GET_FIELD(insn, 7, 12);

            if (xop == 0x3a) {  /* generate trap */

                int cond = GET_FIELD(insn, 3, 6);

                TCGv_i32 trap;

                int l1 = -1, mask;



                if (cond == 0) {

                    /* Trap never.  */

                    break;

                }



                save_state(dc);



                if (cond != 8) {

                    /* Conditional trap.  */

                    DisasCompare cmp;

#ifdef TARGET_SPARC64

                    /* V9 icc/xcc */

                    int cc = GET_FIELD_SP(insn, 11, 12);

                    if (cc == 0) {

                        gen_compare(&cmp, 0, cond, dc);

                    } else if (cc == 2) {

                        gen_compare(&cmp, 1, cond, dc);

                    } else {

                        goto illegal_insn;

                    }

#else

                    gen_compare(&cmp, 0, cond, dc);

#endif

                    l1 = gen_new_label();

                    tcg_gen_brcond_tl(tcg_invert_cond(cmp.cond),

                                      cmp.c1, cmp.c2, l1);

                    free_compare(&cmp);

                }



                mask = ((dc->def->features & CPU_FEATURE_HYPV) && supervisor(dc)

                        ? UA2005_HTRAP_MASK : V8_TRAP_MASK);



                /* Don't use the normal temporaries, as they may well have

                   gone out of scope with the branch above.  While we're

                   doing that we might as well pre-truncate to 32-bit.  */

                trap = tcg_temp_new_i32();



                rs1 = GET_FIELD_SP(insn, 14, 18);

                if (IS_IMM) {

                    rs2 = GET_FIELD_SP(insn, 0, 6);

                    if (rs1 == 0) {

                        tcg_gen_movi_i32(trap, (rs2 & mask) + TT_TRAP);

                        /* Signal that the trap value is fully constant.  */

                        mask = 0;

                    } else {

                        TCGv t1 = gen_load_gpr(dc, rs1);

                        tcg_gen_trunc_tl_i32(trap, t1);

                        tcg_gen_addi_i32(trap, trap, rs2);

                    }

                } else {

                    TCGv t1, t2;

                    rs2 = GET_FIELD_SP(insn, 0, 4);

                    t1 = gen_load_gpr(dc, rs1);

                    t2 = gen_load_gpr(dc, rs2);

                    tcg_gen_add_tl(t1, t1, t2);

                    tcg_gen_trunc_tl_i32(trap, t1);

                }

                if (mask != 0) {

                    tcg_gen_andi_i32(trap, trap, mask);

                    tcg_gen_addi_i32(trap, trap, TT_TRAP);

                }



                gen_helper_raise_exception(cpu_env, trap);

                tcg_temp_free_i32(trap);



                if (cond == 8) {

                    /* An unconditional trap ends the TB.  */

                    dc->is_br = 1;

                    goto jmp_insn;

                } else {

                    /* A conditional trap falls through to the next insn.  */

                    gen_set_label(l1);

                    break;

                }

            } else if (xop == 0x28) {

                rs1 = GET_FIELD(insn, 13, 17);

                switch(rs1) {

                case 0: /* rdy */

#ifndef TARGET_SPARC64

                case 0x01 ... 0x0e: /* undefined in the SPARCv8

                                       manual, rdy on the microSPARC

                                       II */

                case 0x0f:          /* stbar in the SPARCv8 manual,

                                       rdy on the microSPARC II */

                case 0x10 ... 0x1f: /* implementation-dependent in the

                                       SPARCv8 manual, rdy on the

                                       microSPARC II */

                    /* Read Asr17 */

                    if (rs1 == 0x11 && dc->def->features & CPU_FEATURE_ASR17) {

                        TCGv t = gen_dest_gpr(dc, rd);

                        /* Read Asr17 for a Leon3 monoprocessor */

                        tcg_gen_movi_tl(t, (1 << 8) | (dc->def->nwindows - 1));

                        gen_store_gpr(dc, rd, t);

                        break;

                    }

#endif

                    gen_store_gpr(dc, rd, cpu_y);

                    break;

#ifdef TARGET_SPARC64

                case 0x2: /* V9 rdccr */

                    update_psr(dc);

                    gen_helper_rdccr(cpu_dst, cpu_env);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x3: /* V9 rdasi */

                    tcg_gen_ext_i32_tl(cpu_dst, cpu_asi);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x4: /* V9 rdtick */

                    {

                        TCGv_ptr r_tickptr;



                        r_tickptr = tcg_temp_new_ptr();

                        tcg_gen_ld_ptr(r_tickptr, cpu_env,

                                       offsetof(CPUSPARCState, tick));

                        gen_helper_tick_get_count(cpu_dst, r_tickptr);

                        tcg_temp_free_ptr(r_tickptr);

                        gen_store_gpr(dc, rd, cpu_dst);

                    }

                    break;

                case 0x5: /* V9 rdpc */

                    {

                        TCGv t = gen_dest_gpr(dc, rd);

                        if (unlikely(AM_CHECK(dc))) {

                            tcg_gen_movi_tl(t, dc->pc & 0xffffffffULL);

                        } else {

                            tcg_gen_movi_tl(t, dc->pc);

                        }

                        gen_store_gpr(dc, rd, t);

                    }

                    break;

                case 0x6: /* V9 rdfprs */

                    tcg_gen_ext_i32_tl(cpu_dst, cpu_fprs);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0xf: /* V9 membar */

                    break; /* no effect */

                case 0x13: /* Graphics Status */

                    if (gen_trap_ifnofpu(dc)) {

                        goto jmp_insn;

                    }

                    gen_store_gpr(dc, rd, cpu_gsr);

                    break;

                case 0x16: /* Softint */

                    tcg_gen_ext_i32_tl(cpu_dst, cpu_softint);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x17: /* Tick compare */

                    gen_store_gpr(dc, rd, cpu_tick_cmpr);

                    break;

                case 0x18: /* System tick */

                    {

                        TCGv_ptr r_tickptr;



                        r_tickptr = tcg_temp_new_ptr();

                        tcg_gen_ld_ptr(r_tickptr, cpu_env,

                                       offsetof(CPUSPARCState, stick));

                        gen_helper_tick_get_count(cpu_dst, r_tickptr);

                        tcg_temp_free_ptr(r_tickptr);

                        gen_store_gpr(dc, rd, cpu_dst);

                    }

                    break;

                case 0x19: /* System tick compare */

                    gen_store_gpr(dc, rd, cpu_stick_cmpr);

                    break;

                case 0x10: /* Performance Control */

                case 0x11: /* Performance Instrumentation Counter */

                case 0x12: /* Dispatch Control */

                case 0x14: /* Softint set, WO */

                case 0x15: /* Softint clear, WO */

#endif

                default:

                    goto illegal_insn;

                }

#if !defined(CONFIG_USER_ONLY)

            } else if (xop == 0x29) { /* rdpsr / UA2005 rdhpr */

#ifndef TARGET_SPARC64

                if (!supervisor(dc)) {

                    goto priv_insn;

                }

                update_psr(dc);

                gen_helper_rdpsr(cpu_dst, cpu_env);

#else

                CHECK_IU_FEATURE(dc, HYPV);

                if (!hypervisor(dc))

                    goto priv_insn;

                rs1 = GET_FIELD(insn, 13, 17);

                switch (rs1) {

                case 0: // hpstate

                    // gen_op_rdhpstate();

                    break;

                case 1: // htstate

                    // gen_op_rdhtstate();

                    break;

                case 3: // hintp

                    tcg_gen_mov_tl(cpu_dst, cpu_hintp);

                    break;

                case 5: // htba

                    tcg_gen_mov_tl(cpu_dst, cpu_htba);

                    break;

                case 6: // hver

                    tcg_gen_mov_tl(cpu_dst, cpu_hver);

                    break;

                case 31: // hstick_cmpr

                    tcg_gen_mov_tl(cpu_dst, cpu_hstick_cmpr);

                    break;

                default:

                    goto illegal_insn;

                }

#endif

                gen_store_gpr(dc, rd, cpu_dst);

                break;

            } else if (xop == 0x2a) { /* rdwim / V9 rdpr */

                if (!supervisor(dc))

                    goto priv_insn;

#ifdef TARGET_SPARC64

                rs1 = GET_FIELD(insn, 13, 17);

                switch (rs1) {

                case 0: // tpc

                    {

                        TCGv_ptr r_tsptr;



                        r_tsptr = tcg_temp_new_ptr();

                        gen_load_trap_state_at_tl(r_tsptr, cpu_env);

                        tcg_gen_ld_tl(cpu_tmp0, r_tsptr,

                                      offsetof(trap_state, tpc));

                        tcg_temp_free_ptr(r_tsptr);

                    }

                    break;

                case 1: // tnpc

                    {

                        TCGv_ptr r_tsptr;



                        r_tsptr = tcg_temp_new_ptr();

                        gen_load_trap_state_at_tl(r_tsptr, cpu_env);

                        tcg_gen_ld_tl(cpu_tmp0, r_tsptr,

                                      offsetof(trap_state, tnpc));

                        tcg_temp_free_ptr(r_tsptr);

                    }

                    break;

                case 2: // tstate

                    {

                        TCGv_ptr r_tsptr;



                        r_tsptr = tcg_temp_new_ptr();

                        gen_load_trap_state_at_tl(r_tsptr, cpu_env);

                        tcg_gen_ld_tl(cpu_tmp0, r_tsptr,

                                      offsetof(trap_state, tstate));

                        tcg_temp_free_ptr(r_tsptr);

                    }

                    break;

                case 3: // tt

                    {

                        TCGv_ptr r_tsptr = tcg_temp_new_ptr();



                        gen_load_trap_state_at_tl(r_tsptr, cpu_env);

                        tcg_gen_ld32s_tl(cpu_tmp0, r_tsptr,

                                         offsetof(trap_state, tt));

                        tcg_temp_free_ptr(r_tsptr);

                    }

                    break;

                case 4: // tick

                    {

                        TCGv_ptr r_tickptr;



                        r_tickptr = tcg_temp_new_ptr();

                        tcg_gen_ld_ptr(r_tickptr, cpu_env,

                                       offsetof(CPUSPARCState, tick));

                        gen_helper_tick_get_count(cpu_tmp0, r_tickptr);

                        tcg_temp_free_ptr(r_tickptr);

                    }

                    break;

                case 5: // tba

                    tcg_gen_mov_tl(cpu_tmp0, cpu_tbr);

                    break;

                case 6: // pstate

                    tcg_gen_ld32s_tl(cpu_tmp0, cpu_env,

                                     offsetof(CPUSPARCState, pstate));

                    break;

                case 7: // tl

                    tcg_gen_ld32s_tl(cpu_tmp0, cpu_env,

                                     offsetof(CPUSPARCState, tl));

                    break;

                case 8: // pil

                    tcg_gen_ld32s_tl(cpu_tmp0, cpu_env,

                                     offsetof(CPUSPARCState, psrpil));

                    break;

                case 9: // cwp

                    gen_helper_rdcwp(cpu_tmp0, cpu_env);

                    break;

                case 10: // cansave

                    tcg_gen_ld32s_tl(cpu_tmp0, cpu_env,

                                     offsetof(CPUSPARCState, cansave));

                    break;

                case 11: // canrestore

                    tcg_gen_ld32s_tl(cpu_tmp0, cpu_env,

                                     offsetof(CPUSPARCState, canrestore));

                    break;

                case 12: // cleanwin

                    tcg_gen_ld32s_tl(cpu_tmp0, cpu_env,

                                     offsetof(CPUSPARCState, cleanwin));

                    break;

                case 13: // otherwin

                    tcg_gen_ld32s_tl(cpu_tmp0, cpu_env,

                                     offsetof(CPUSPARCState, otherwin));

                    break;

                case 14: // wstate

                    tcg_gen_ld32s_tl(cpu_tmp0, cpu_env,

                                     offsetof(CPUSPARCState, wstate));

                    break;

                case 16: // UA2005 gl

                    CHECK_IU_FEATURE(dc, GL);

                    tcg_gen_ld32s_tl(cpu_tmp0, cpu_env,

                                     offsetof(CPUSPARCState, gl));

                    break;

                case 26: // UA2005 strand status

                    CHECK_IU_FEATURE(dc, HYPV);

                    if (!hypervisor(dc))

                        goto priv_insn;

                    tcg_gen_mov_tl(cpu_tmp0, cpu_ssr);

                    break;

                case 31: // ver

                    tcg_gen_mov_tl(cpu_tmp0, cpu_ver);

                    break;

                case 15: // fq

                default:

                    goto illegal_insn;

                }

#else

                tcg_gen_ext_i32_tl(cpu_tmp0, cpu_wim);

#endif

                gen_store_gpr(dc, rd, cpu_tmp0);

                break;

            } else if (xop == 0x2b) { /* rdtbr / V9 flushw */

#ifdef TARGET_SPARC64

                save_state(dc);

                gen_helper_flushw(cpu_env);

#else

                if (!supervisor(dc))

                    goto priv_insn;

                gen_store_gpr(dc, rd, cpu_tbr);

#endif

                break;

#endif

            } else if (xop == 0x34) {   /* FPU Operations */

                if (gen_trap_ifnofpu(dc)) {

                    goto jmp_insn;

                }

                gen_op_clear_ieee_excp_and_FTT();

                rs1 = GET_FIELD(insn, 13, 17);

                rs2 = GET_FIELD(insn, 27, 31);

                xop = GET_FIELD(insn, 18, 26);

                save_state(dc);

                switch (xop) {

                case 0x1: /* fmovs */

                    cpu_src1_32 = gen_load_fpr_F(dc, rs2);

                    gen_store_fpr_F(dc, rd, cpu_src1_32);

                    break;

                case 0x5: /* fnegs */

                    gen_ne_fop_FF(dc, rd, rs2, gen_helper_fnegs);

                    break;

                case 0x9: /* fabss */

                    gen_ne_fop_FF(dc, rd, rs2, gen_helper_fabss);

                    break;

                case 0x29: /* fsqrts */

                    CHECK_FPU_FEATURE(dc, FSQRT);

                    gen_fop_FF(dc, rd, rs2, gen_helper_fsqrts);

                    break;

                case 0x2a: /* fsqrtd */

                    CHECK_FPU_FEATURE(dc, FSQRT);

                    gen_fop_DD(dc, rd, rs2, gen_helper_fsqrtd);

                    break;

                case 0x2b: /* fsqrtq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_fop_QQ(dc, rd, rs2, gen_helper_fsqrtq);

                    break;

                case 0x41: /* fadds */

                    gen_fop_FFF(dc, rd, rs1, rs2, gen_helper_fadds);

                    break;

                case 0x42: /* faddd */

                    gen_fop_DDD(dc, rd, rs1, rs2, gen_helper_faddd);

                    break;

                case 0x43: /* faddq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_fop_QQQ(dc, rd, rs1, rs2, gen_helper_faddq);

                    break;

                case 0x45: /* fsubs */

                    gen_fop_FFF(dc, rd, rs1, rs2, gen_helper_fsubs);

                    break;

                case 0x46: /* fsubd */

                    gen_fop_DDD(dc, rd, rs1, rs2, gen_helper_fsubd);

                    break;

                case 0x47: /* fsubq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_fop_QQQ(dc, rd, rs1, rs2, gen_helper_fsubq);

                    break;

                case 0x49: /* fmuls */

                    CHECK_FPU_FEATURE(dc, FMUL);

                    gen_fop_FFF(dc, rd, rs1, rs2, gen_helper_fmuls);

                    break;

                case 0x4a: /* fmuld */

                    CHECK_FPU_FEATURE(dc, FMUL);

                    gen_fop_DDD(dc, rd, rs1, rs2, gen_helper_fmuld);

                    break;

                case 0x4b: /* fmulq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    CHECK_FPU_FEATURE(dc, FMUL);

                    gen_fop_QQQ(dc, rd, rs1, rs2, gen_helper_fmulq);

                    break;

                case 0x4d: /* fdivs */

                    gen_fop_FFF(dc, rd, rs1, rs2, gen_helper_fdivs);

                    break;

                case 0x4e: /* fdivd */

                    gen_fop_DDD(dc, rd, rs1, rs2, gen_helper_fdivd);

                    break;

                case 0x4f: /* fdivq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_fop_QQQ(dc, rd, rs1, rs2, gen_helper_fdivq);

                    break;

                case 0x69: /* fsmuld */

                    CHECK_FPU_FEATURE(dc, FSMULD);

                    gen_fop_DFF(dc, rd, rs1, rs2, gen_helper_fsmuld);

                    break;

                case 0x6e: /* fdmulq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_fop_QDD(dc, rd, rs1, rs2, gen_helper_fdmulq);

                    break;

                case 0xc4: /* fitos */

                    gen_fop_FF(dc, rd, rs2, gen_helper_fitos);

                    break;

                case 0xc6: /* fdtos */

                    gen_fop_FD(dc, rd, rs2, gen_helper_fdtos);

                    break;

                case 0xc7: /* fqtos */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_fop_FQ(dc, rd, rs2, gen_helper_fqtos);

                    break;

                case 0xc8: /* fitod */

                    gen_ne_fop_DF(dc, rd, rs2, gen_helper_fitod);

                    break;

                case 0xc9: /* fstod */

                    gen_ne_fop_DF(dc, rd, rs2, gen_helper_fstod);

                    break;

                case 0xcb: /* fqtod */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_fop_DQ(dc, rd, rs2, gen_helper_fqtod);

                    break;

                case 0xcc: /* fitoq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_ne_fop_QF(dc, rd, rs2, gen_helper_fitoq);

                    break;

                case 0xcd: /* fstoq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_ne_fop_QF(dc, rd, rs2, gen_helper_fstoq);

                    break;

                case 0xce: /* fdtoq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_ne_fop_QD(dc, rd, rs2, gen_helper_fdtoq);

                    break;

                case 0xd1: /* fstoi */

                    gen_fop_FF(dc, rd, rs2, gen_helper_fstoi);

                    break;

                case 0xd2: /* fdtoi */

                    gen_fop_FD(dc, rd, rs2, gen_helper_fdtoi);

                    break;

                case 0xd3: /* fqtoi */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_fop_FQ(dc, rd, rs2, gen_helper_fqtoi);

                    break;

#ifdef TARGET_SPARC64

                case 0x2: /* V9 fmovd */

                    cpu_src1_64 = gen_load_fpr_D(dc, rs2);

                    gen_store_fpr_D(dc, rd, cpu_src1_64);

                    break;

                case 0x3: /* V9 fmovq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_move_Q(rd, rs2);

                    break;

                case 0x6: /* V9 fnegd */

                    gen_ne_fop_DD(dc, rd, rs2, gen_helper_fnegd);

                    break;

                case 0x7: /* V9 fnegq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_ne_fop_QQ(dc, rd, rs2, gen_helper_fnegq);

                    break;

                case 0xa: /* V9 fabsd */

                    gen_ne_fop_DD(dc, rd, rs2, gen_helper_fabsd);

                    break;

                case 0xb: /* V9 fabsq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_ne_fop_QQ(dc, rd, rs2, gen_helper_fabsq);

                    break;

                case 0x81: /* V9 fstox */

                    gen_fop_DF(dc, rd, rs2, gen_helper_fstox);

                    break;

                case 0x82: /* V9 fdtox */

                    gen_fop_DD(dc, rd, rs2, gen_helper_fdtox);

                    break;

                case 0x83: /* V9 fqtox */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_fop_DQ(dc, rd, rs2, gen_helper_fqtox);

                    break;

                case 0x84: /* V9 fxtos */

                    gen_fop_FD(dc, rd, rs2, gen_helper_fxtos);

                    break;

                case 0x88: /* V9 fxtod */

                    gen_fop_DD(dc, rd, rs2, gen_helper_fxtod);

                    break;

                case 0x8c: /* V9 fxtoq */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    gen_ne_fop_QD(dc, rd, rs2, gen_helper_fxtoq);

                    break;

#endif

                default:

                    goto illegal_insn;

                }

            } else if (xop == 0x35) {   /* FPU Operations */

#ifdef TARGET_SPARC64

                int cond;

#endif

                if (gen_trap_ifnofpu(dc)) {

                    goto jmp_insn;

                }

                gen_op_clear_ieee_excp_and_FTT();

                rs1 = GET_FIELD(insn, 13, 17);

                rs2 = GET_FIELD(insn, 27, 31);

                xop = GET_FIELD(insn, 18, 26);

                save_state(dc);



#ifdef TARGET_SPARC64

#define FMOVR(sz)                                                  \

                do {                                               \

                    DisasCompare cmp;                              \

                    cond = GET_FIELD_SP(insn, 14, 17);             \

                    cpu_src1 = get_src1(dc, insn);                 \

                    gen_compare_reg(&cmp, cond, cpu_src1);         \

                    gen_fmov##sz(dc, &cmp, rd, rs2);               \

                    free_compare(&cmp);                            \

                } while (0)



                if ((xop & 0x11f) == 0x005) { /* V9 fmovsr */

                    FMOVR(s);

                    break;

                } else if ((xop & 0x11f) == 0x006) { // V9 fmovdr

                    FMOVR(d);

                    break;

                } else if ((xop & 0x11f) == 0x007) { // V9 fmovqr

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    FMOVR(q);

                    break;

                }

#undef FMOVR

#endif

                switch (xop) {

#ifdef TARGET_SPARC64

#define FMOVCC(fcc, sz)                                                 \

                    do {                                                \

                        DisasCompare cmp;                               \

                        cond = GET_FIELD_SP(insn, 14, 17);              \

                        gen_fcompare(&cmp, fcc, cond);                  \

                        gen_fmov##sz(dc, &cmp, rd, rs2);                \

                        free_compare(&cmp);                             \

                    } while (0)



                    case 0x001: /* V9 fmovscc %fcc0 */

                        FMOVCC(0, s);

                        break;

                    case 0x002: /* V9 fmovdcc %fcc0 */

                        FMOVCC(0, d);

                        break;

                    case 0x003: /* V9 fmovqcc %fcc0 */

                        CHECK_FPU_FEATURE(dc, FLOAT128);

                        FMOVCC(0, q);

                        break;

                    case 0x041: /* V9 fmovscc %fcc1 */

                        FMOVCC(1, s);

                        break;

                    case 0x042: /* V9 fmovdcc %fcc1 */

                        FMOVCC(1, d);

                        break;

                    case 0x043: /* V9 fmovqcc %fcc1 */

                        CHECK_FPU_FEATURE(dc, FLOAT128);

                        FMOVCC(1, q);

                        break;

                    case 0x081: /* V9 fmovscc %fcc2 */

                        FMOVCC(2, s);

                        break;

                    case 0x082: /* V9 fmovdcc %fcc2 */

                        FMOVCC(2, d);

                        break;

                    case 0x083: /* V9 fmovqcc %fcc2 */

                        CHECK_FPU_FEATURE(dc, FLOAT128);

                        FMOVCC(2, q);

                        break;

                    case 0x0c1: /* V9 fmovscc %fcc3 */

                        FMOVCC(3, s);

                        break;

                    case 0x0c2: /* V9 fmovdcc %fcc3 */

                        FMOVCC(3, d);

                        break;

                    case 0x0c3: /* V9 fmovqcc %fcc3 */

                        CHECK_FPU_FEATURE(dc, FLOAT128);

                        FMOVCC(3, q);

                        break;

#undef FMOVCC

#define FMOVCC(xcc, sz)                                                 \

                    do {                                                \

                        DisasCompare cmp;                               \

                        cond = GET_FIELD_SP(insn, 14, 17);              \

                        gen_compare(&cmp, xcc, cond, dc);               \

                        gen_fmov##sz(dc, &cmp, rd, rs2);                \

                        free_compare(&cmp);                             \

                    } while (0)



                    case 0x101: /* V9 fmovscc %icc */

                        FMOVCC(0, s);

                        break;

                    case 0x102: /* V9 fmovdcc %icc */

                        FMOVCC(0, d);

                        break;

                    case 0x103: /* V9 fmovqcc %icc */

                        CHECK_FPU_FEATURE(dc, FLOAT128);

                        FMOVCC(0, q);

                        break;

                    case 0x181: /* V9 fmovscc %xcc */

                        FMOVCC(1, s);

                        break;

                    case 0x182: /* V9 fmovdcc %xcc */

                        FMOVCC(1, d);

                        break;

                    case 0x183: /* V9 fmovqcc %xcc */

                        CHECK_FPU_FEATURE(dc, FLOAT128);

                        FMOVCC(1, q);

                        break;

#undef FMOVCC

#endif

                    case 0x51: /* fcmps, V9 %fcc */

                        cpu_src1_32 = gen_load_fpr_F(dc, rs1);

                        cpu_src2_32 = gen_load_fpr_F(dc, rs2);

                        gen_op_fcmps(rd & 3, cpu_src1_32, cpu_src2_32);

                        break;

                    case 0x52: /* fcmpd, V9 %fcc */

                        cpu_src1_64 = gen_load_fpr_D(dc, rs1);

                        cpu_src2_64 = gen_load_fpr_D(dc, rs2);

                        gen_op_fcmpd(rd & 3, cpu_src1_64, cpu_src2_64);

                        break;

                    case 0x53: /* fcmpq, V9 %fcc */

                        CHECK_FPU_FEATURE(dc, FLOAT128);

                        gen_op_load_fpr_QT0(QFPREG(rs1));

                        gen_op_load_fpr_QT1(QFPREG(rs2));

                        gen_op_fcmpq(rd & 3);

                        break;

                    case 0x55: /* fcmpes, V9 %fcc */

                        cpu_src1_32 = gen_load_fpr_F(dc, rs1);

                        cpu_src2_32 = gen_load_fpr_F(dc, rs2);

                        gen_op_fcmpes(rd & 3, cpu_src1_32, cpu_src2_32);

                        break;

                    case 0x56: /* fcmped, V9 %fcc */

                        cpu_src1_64 = gen_load_fpr_D(dc, rs1);

                        cpu_src2_64 = gen_load_fpr_D(dc, rs2);

                        gen_op_fcmped(rd & 3, cpu_src1_64, cpu_src2_64);

                        break;

                    case 0x57: /* fcmpeq, V9 %fcc */

                        CHECK_FPU_FEATURE(dc, FLOAT128);

                        gen_op_load_fpr_QT0(QFPREG(rs1));

                        gen_op_load_fpr_QT1(QFPREG(rs2));

                        gen_op_fcmpeq(rd & 3);

                        break;

                    default:

                        goto illegal_insn;

                }

            } else if (xop == 0x2) {

                TCGv dst = gen_dest_gpr(dc, rd);

                rs1 = GET_FIELD(insn, 13, 17);

                if (rs1 == 0) {

                    /* clr/mov shortcut : or %g0, x, y -> mov x, y */

                    if (IS_IMM) {       /* immediate */

                        simm = GET_FIELDs(insn, 19, 31);

                        tcg_gen_movi_tl(dst, simm);

                        gen_store_gpr(dc, rd, dst);

                    } else {            /* register */

                        rs2 = GET_FIELD(insn, 27, 31);

                        if (rs2 == 0) {

                            tcg_gen_movi_tl(dst, 0);

                            gen_store_gpr(dc, rd, dst);

                        } else {

                            cpu_src2 = gen_load_gpr(dc, rs2);

                            gen_store_gpr(dc, rd, cpu_src2);

                        }

                    }

                } else {

                    cpu_src1 = get_src1(dc, insn);

                    if (IS_IMM) {       /* immediate */

                        simm = GET_FIELDs(insn, 19, 31);

                        tcg_gen_ori_tl(dst, cpu_src1, simm);

                        gen_store_gpr(dc, rd, dst);

                    } else {            /* register */

                        rs2 = GET_FIELD(insn, 27, 31);

                        if (rs2 == 0) {

                            /* mov shortcut:  or x, %g0, y -> mov x, y */

                            gen_store_gpr(dc, rd, cpu_src1);

                        } else {

                            cpu_src2 = gen_load_gpr(dc, rs2);

                            tcg_gen_or_tl(dst, cpu_src1, cpu_src2);

                            gen_store_gpr(dc, rd, dst);

                        }

                    }

                }

#ifdef TARGET_SPARC64

            } else if (xop == 0x25) { /* sll, V9 sllx */

                cpu_src1 = get_src1(dc, insn);

                if (IS_IMM) {   /* immediate */

                    simm = GET_FIELDs(insn, 20, 31);

                    if (insn & (1 << 12)) {

                        tcg_gen_shli_i64(cpu_dst, cpu_src1, simm & 0x3f);

                    } else {

                        tcg_gen_shli_i64(cpu_dst, cpu_src1, simm & 0x1f);

                    }

                } else {                /* register */

                    rs2 = GET_FIELD(insn, 27, 31);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    if (insn & (1 << 12)) {

                        tcg_gen_andi_i64(cpu_tmp0, cpu_src2, 0x3f);

                    } else {

                        tcg_gen_andi_i64(cpu_tmp0, cpu_src2, 0x1f);

                    }

                    tcg_gen_shl_i64(cpu_dst, cpu_src1, cpu_tmp0);

                }

                gen_store_gpr(dc, rd, cpu_dst);

            } else if (xop == 0x26) { /* srl, V9 srlx */

                cpu_src1 = get_src1(dc, insn);

                if (IS_IMM) {   /* immediate */

                    simm = GET_FIELDs(insn, 20, 31);

                    if (insn & (1 << 12)) {

                        tcg_gen_shri_i64(cpu_dst, cpu_src1, simm & 0x3f);

                    } else {

                        tcg_gen_andi_i64(cpu_dst, cpu_src1, 0xffffffffULL);

                        tcg_gen_shri_i64(cpu_dst, cpu_dst, simm & 0x1f);

                    }

                } else {                /* register */

                    rs2 = GET_FIELD(insn, 27, 31);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    if (insn & (1 << 12)) {

                        tcg_gen_andi_i64(cpu_tmp0, cpu_src2, 0x3f);

                        tcg_gen_shr_i64(cpu_dst, cpu_src1, cpu_tmp0);

                    } else {

                        tcg_gen_andi_i64(cpu_tmp0, cpu_src2, 0x1f);

                        tcg_gen_andi_i64(cpu_dst, cpu_src1, 0xffffffffULL);

                        tcg_gen_shr_i64(cpu_dst, cpu_dst, cpu_tmp0);

                    }

                }

                gen_store_gpr(dc, rd, cpu_dst);

            } else if (xop == 0x27) { /* sra, V9 srax */

                cpu_src1 = get_src1(dc, insn);

                if (IS_IMM) {   /* immediate */

                    simm = GET_FIELDs(insn, 20, 31);

                    if (insn & (1 << 12)) {

                        tcg_gen_sari_i64(cpu_dst, cpu_src1, simm & 0x3f);

                    } else {

                        tcg_gen_ext32s_i64(cpu_dst, cpu_src1);

                        tcg_gen_sari_i64(cpu_dst, cpu_dst, simm & 0x1f);

                    }

                } else {                /* register */

                    rs2 = GET_FIELD(insn, 27, 31);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    if (insn & (1 << 12)) {

                        tcg_gen_andi_i64(cpu_tmp0, cpu_src2, 0x3f);

                        tcg_gen_sar_i64(cpu_dst, cpu_src1, cpu_tmp0);

                    } else {

                        tcg_gen_andi_i64(cpu_tmp0, cpu_src2, 0x1f);

                        tcg_gen_ext32s_i64(cpu_dst, cpu_src1);

                        tcg_gen_sar_i64(cpu_dst, cpu_dst, cpu_tmp0);

                    }

                }

                gen_store_gpr(dc, rd, cpu_dst);

#endif

            } else if (xop < 0x36) {

                if (xop < 0x20) {

                    cpu_src1 = get_src1(dc, insn);

                    cpu_src2 = get_src2(dc, insn);

                    switch (xop & ~0x10) {

                    case 0x0: /* add */

                        if (xop & 0x10) {

                            gen_op_add_cc(cpu_dst, cpu_src1, cpu_src2);

                            tcg_gen_movi_i32(cpu_cc_op, CC_OP_ADD);

                            dc->cc_op = CC_OP_ADD;

                        } else {

                            tcg_gen_add_tl(cpu_dst, cpu_src1, cpu_src2);

                        }

                        break;

                    case 0x1: /* and */

                        tcg_gen_and_tl(cpu_dst, cpu_src1, cpu_src2);

                        if (xop & 0x10) {

                            tcg_gen_mov_tl(cpu_cc_dst, cpu_dst);

                            tcg_gen_movi_i32(cpu_cc_op, CC_OP_LOGIC);

                            dc->cc_op = CC_OP_LOGIC;

                        }

                        break;

                    case 0x2: /* or */

                        tcg_gen_or_tl(cpu_dst, cpu_src1, cpu_src2);

                        if (xop & 0x10) {

                            tcg_gen_mov_tl(cpu_cc_dst, cpu_dst);

                            tcg_gen_movi_i32(cpu_cc_op, CC_OP_LOGIC);

                            dc->cc_op = CC_OP_LOGIC;

                        }

                        break;

                    case 0x3: /* xor */

                        tcg_gen_xor_tl(cpu_dst, cpu_src1, cpu_src2);

                        if (xop & 0x10) {

                            tcg_gen_mov_tl(cpu_cc_dst, cpu_dst);

                            tcg_gen_movi_i32(cpu_cc_op, CC_OP_LOGIC);

                            dc->cc_op = CC_OP_LOGIC;

                        }

                        break;

                    case 0x4: /* sub */

                        if (xop & 0x10) {

                            gen_op_sub_cc(cpu_dst, cpu_src1, cpu_src2);

                            tcg_gen_movi_i32(cpu_cc_op, CC_OP_SUB);

                            dc->cc_op = CC_OP_SUB;

                        } else {

                            tcg_gen_sub_tl(cpu_dst, cpu_src1, cpu_src2);

                        }

                        break;

                    case 0x5: /* andn */

                        tcg_gen_andc_tl(cpu_dst, cpu_src1, cpu_src2);

                        if (xop & 0x10) {

                            tcg_gen_mov_tl(cpu_cc_dst, cpu_dst);

                            tcg_gen_movi_i32(cpu_cc_op, CC_OP_LOGIC);

                            dc->cc_op = CC_OP_LOGIC;

                        }

                        break;

                    case 0x6: /* orn */

                        tcg_gen_orc_tl(cpu_dst, cpu_src1, cpu_src2);

                        if (xop & 0x10) {

                            tcg_gen_mov_tl(cpu_cc_dst, cpu_dst);

                            tcg_gen_movi_i32(cpu_cc_op, CC_OP_LOGIC);

                            dc->cc_op = CC_OP_LOGIC;

                        }

                        break;

                    case 0x7: /* xorn */

                        tcg_gen_eqv_tl(cpu_dst, cpu_src1, cpu_src2);

                        if (xop & 0x10) {

                            tcg_gen_mov_tl(cpu_cc_dst, cpu_dst);

                            tcg_gen_movi_i32(cpu_cc_op, CC_OP_LOGIC);

                            dc->cc_op = CC_OP_LOGIC;

                        }

                        break;

                    case 0x8: /* addx, V9 addc */

                        gen_op_addx_int(dc, cpu_dst, cpu_src1, cpu_src2,

                                        (xop & 0x10));

                        break;

#ifdef TARGET_SPARC64

                    case 0x9: /* V9 mulx */

                        tcg_gen_mul_i64(cpu_dst, cpu_src1, cpu_src2);

                        break;

#endif

                    case 0xa: /* umul */

                        CHECK_IU_FEATURE(dc, MUL);

                        gen_op_umul(cpu_dst, cpu_src1, cpu_src2);

                        if (xop & 0x10) {

                            tcg_gen_mov_tl(cpu_cc_dst, cpu_dst);

                            tcg_gen_movi_i32(cpu_cc_op, CC_OP_LOGIC);

                            dc->cc_op = CC_OP_LOGIC;

                        }

                        break;

                    case 0xb: /* smul */

                        CHECK_IU_FEATURE(dc, MUL);

                        gen_op_smul(cpu_dst, cpu_src1, cpu_src2);

                        if (xop & 0x10) {

                            tcg_gen_mov_tl(cpu_cc_dst, cpu_dst);

                            tcg_gen_movi_i32(cpu_cc_op, CC_OP_LOGIC);

                            dc->cc_op = CC_OP_LOGIC;

                        }

                        break;

                    case 0xc: /* subx, V9 subc */

                        gen_op_subx_int(dc, cpu_dst, cpu_src1, cpu_src2,

                                        (xop & 0x10));

                        break;

#ifdef TARGET_SPARC64

                    case 0xd: /* V9 udivx */

                        gen_helper_udivx(cpu_dst, cpu_env, cpu_src1, cpu_src2);

                        break;

#endif

                    case 0xe: /* udiv */

                        CHECK_IU_FEATURE(dc, DIV);

                        if (xop & 0x10) {

                            gen_helper_udiv_cc(cpu_dst, cpu_env, cpu_src1,

                                               cpu_src2);

                            dc->cc_op = CC_OP_DIV;

                        } else {

                            gen_helper_udiv(cpu_dst, cpu_env, cpu_src1,

                                            cpu_src2);

                        }

                        break;

                    case 0xf: /* sdiv */

                        CHECK_IU_FEATURE(dc, DIV);

                        if (xop & 0x10) {

                            gen_helper_sdiv_cc(cpu_dst, cpu_env, cpu_src1,

                                               cpu_src2);

                            dc->cc_op = CC_OP_DIV;

                        } else {

                            gen_helper_sdiv(cpu_dst, cpu_env, cpu_src1,

                                            cpu_src2);

                        }

                        break;

                    default:

                        goto illegal_insn;

                    }

                    gen_store_gpr(dc, rd, cpu_dst);

                } else {

                    cpu_src1 = get_src1(dc, insn);

                    cpu_src2 = get_src2(dc, insn);

                    switch (xop) {

                    case 0x20: /* taddcc */

                        gen_op_add_cc(cpu_dst, cpu_src1, cpu_src2);

                        gen_store_gpr(dc, rd, cpu_dst);

                        tcg_gen_movi_i32(cpu_cc_op, CC_OP_TADD);

                        dc->cc_op = CC_OP_TADD;

                        break;

                    case 0x21: /* tsubcc */

                        gen_op_sub_cc(cpu_dst, cpu_src1, cpu_src2);

                        gen_store_gpr(dc, rd, cpu_dst);

                        tcg_gen_movi_i32(cpu_cc_op, CC_OP_TSUB);

                        dc->cc_op = CC_OP_TSUB;

                        break;

                    case 0x22: /* taddcctv */

                        gen_helper_taddcctv(cpu_dst, cpu_env,

                                            cpu_src1, cpu_src2);

                        gen_store_gpr(dc, rd, cpu_dst);

                        dc->cc_op = CC_OP_TADDTV;

                        break;

                    case 0x23: /* tsubcctv */

                        gen_helper_tsubcctv(cpu_dst, cpu_env,

                                            cpu_src1, cpu_src2);

                        gen_store_gpr(dc, rd, cpu_dst);

                        dc->cc_op = CC_OP_TSUBTV;

                        break;

                    case 0x24: /* mulscc */

                        update_psr(dc);

                        gen_op_mulscc(cpu_dst, cpu_src1, cpu_src2);

                        gen_store_gpr(dc, rd, cpu_dst);

                        tcg_gen_movi_i32(cpu_cc_op, CC_OP_ADD);

                        dc->cc_op = CC_OP_ADD;

                        break;

#ifndef TARGET_SPARC64

                    case 0x25:  /* sll */

                        if (IS_IMM) { /* immediate */

                            simm = GET_FIELDs(insn, 20, 31);

                            tcg_gen_shli_tl(cpu_dst, cpu_src1, simm & 0x1f);

                        } else { /* register */

                            tcg_gen_andi_tl(cpu_tmp0, cpu_src2, 0x1f);

                            tcg_gen_shl_tl(cpu_dst, cpu_src1, cpu_tmp0);

                        }

                        gen_store_gpr(dc, rd, cpu_dst);

                        break;

                    case 0x26:  /* srl */

                        if (IS_IMM) { /* immediate */

                            simm = GET_FIELDs(insn, 20, 31);

                            tcg_gen_shri_tl(cpu_dst, cpu_src1, simm & 0x1f);

                        } else { /* register */

                            tcg_gen_andi_tl(cpu_tmp0, cpu_src2, 0x1f);

                            tcg_gen_shr_tl(cpu_dst, cpu_src1, cpu_tmp0);

                        }

                        gen_store_gpr(dc, rd, cpu_dst);

                        break;

                    case 0x27:  /* sra */

                        if (IS_IMM) { /* immediate */

                            simm = GET_FIELDs(insn, 20, 31);

                            tcg_gen_sari_tl(cpu_dst, cpu_src1, simm & 0x1f);

                        } else { /* register */

                            tcg_gen_andi_tl(cpu_tmp0, cpu_src2, 0x1f);

                            tcg_gen_sar_tl(cpu_dst, cpu_src1, cpu_tmp0);

                        }

                        gen_store_gpr(dc, rd, cpu_dst);

                        break;

#endif

                    case 0x30:

                        {

                            switch(rd) {

                            case 0: /* wry */

                                tcg_gen_xor_tl(cpu_tmp0, cpu_src1, cpu_src2);

                                tcg_gen_andi_tl(cpu_y, cpu_tmp0, 0xffffffff);

                                break;

#ifndef TARGET_SPARC64

                            case 0x01 ... 0x0f: /* undefined in the

                                                   SPARCv8 manual, nop

                                                   on the microSPARC

                                                   II */

                            case 0x10 ... 0x1f: /* implementation-dependent

                                                   in the SPARCv8

                                                   manual, nop on the

                                                   microSPARC II */

                                break;

#else

                            case 0x2: /* V9 wrccr */

                                tcg_gen_xor_tl(cpu_tmp0, cpu_src1, cpu_src2);

                                gen_helper_wrccr(cpu_env, cpu_tmp0);

                                tcg_gen_movi_i32(cpu_cc_op, CC_OP_FLAGS);

                                dc->cc_op = CC_OP_FLAGS;

                                break;

                            case 0x3: /* V9 wrasi */

                                tcg_gen_xor_tl(cpu_tmp0, cpu_src1, cpu_src2);

                                tcg_gen_andi_tl(cpu_tmp0, cpu_tmp0, 0xff);

                                tcg_gen_trunc_tl_i32(cpu_asi, cpu_tmp0);

                                break;

                            case 0x6: /* V9 wrfprs */

                                tcg_gen_xor_tl(cpu_tmp0, cpu_src1, cpu_src2);

                                tcg_gen_trunc_tl_i32(cpu_fprs, cpu_tmp0);

                                save_state(dc);

                                gen_op_next_insn();

                                tcg_gen_exit_tb(0);

                                dc->is_br = 1;

                                break;

                            case 0xf: /* V9 sir, nop if user */

#if !defined(CONFIG_USER_ONLY)

                                if (supervisor(dc)) {

                                    ; // XXX

                                }

#endif

                                break;

                            case 0x13: /* Graphics Status */

                                if (gen_trap_ifnofpu(dc)) {

                                    goto jmp_insn;

                                }

                                tcg_gen_xor_tl(cpu_gsr, cpu_src1, cpu_src2);

                                break;

                            case 0x14: /* Softint set */

                                if (!supervisor(dc))

                                    goto illegal_insn;

                                tcg_gen_xor_tl(cpu_tmp0, cpu_src1, cpu_src2);

                                gen_helper_set_softint(cpu_env, cpu_tmp0);

                                break;

                            case 0x15: /* Softint clear */

                                if (!supervisor(dc))

                                    goto illegal_insn;

                                tcg_gen_xor_tl(cpu_tmp0, cpu_src1, cpu_src2);

                                gen_helper_clear_softint(cpu_env, cpu_tmp0);

                                break;

                            case 0x16: /* Softint write */

                                if (!supervisor(dc))

                                    goto illegal_insn;

                                tcg_gen_xor_tl(cpu_tmp0, cpu_src1, cpu_src2);

                                gen_helper_write_softint(cpu_env, cpu_tmp0);

                                break;

                            case 0x17: /* Tick compare */

#if !defined(CONFIG_USER_ONLY)

                                if (!supervisor(dc))

                                    goto illegal_insn;

#endif

                                {

                                    TCGv_ptr r_tickptr;



                                    tcg_gen_xor_tl(cpu_tick_cmpr, cpu_src1,

                                                   cpu_src2);

                                    r_tickptr = tcg_temp_new_ptr();

                                    tcg_gen_ld_ptr(r_tickptr, cpu_env,

                                                   offsetof(CPUSPARCState, tick));

                                    gen_helper_tick_set_limit(r_tickptr,

                                                              cpu_tick_cmpr);

                                    tcg_temp_free_ptr(r_tickptr);

                                }

                                break;

                            case 0x18: /* System tick */

#if !defined(CONFIG_USER_ONLY)

                                if (!supervisor(dc))

                                    goto illegal_insn;

#endif

                                {

                                    TCGv_ptr r_tickptr;



                                    tcg_gen_xor_tl(cpu_tmp0, cpu_src1,

                                                   cpu_src2);

                                    r_tickptr = tcg_temp_new_ptr();

                                    tcg_gen_ld_ptr(r_tickptr, cpu_env,

                                                   offsetof(CPUSPARCState, stick));

                                    gen_helper_tick_set_count(r_tickptr,

                                                              cpu_tmp0);

                                    tcg_temp_free_ptr(r_tickptr);

                                }

                                break;

                            case 0x19: /* System tick compare */

#if !defined(CONFIG_USER_ONLY)

                                if (!supervisor(dc))

                                    goto illegal_insn;

#endif

                                {

                                    TCGv_ptr r_tickptr;



                                    tcg_gen_xor_tl(cpu_stick_cmpr, cpu_src1,

                                                   cpu_src2);

                                    r_tickptr = tcg_temp_new_ptr();

                                    tcg_gen_ld_ptr(r_tickptr, cpu_env,

                                                   offsetof(CPUSPARCState, stick));

                                    gen_helper_tick_set_limit(r_tickptr,

                                                              cpu_stick_cmpr);

                                    tcg_temp_free_ptr(r_tickptr);

                                }

                                break;



                            case 0x10: /* Performance Control */

                            case 0x11: /* Performance Instrumentation

                                          Counter */

                            case 0x12: /* Dispatch Control */

#endif

                            default:

                                goto illegal_insn;

                            }

                        }

                        break;

#if !defined(CONFIG_USER_ONLY)

                    case 0x31: /* wrpsr, V9 saved, restored */

                        {

                            if (!supervisor(dc))

                                goto priv_insn;

#ifdef TARGET_SPARC64

                            switch (rd) {

                            case 0:

                                gen_helper_saved(cpu_env);

                                break;

                            case 1:

                                gen_helper_restored(cpu_env);

                                break;

                            case 2: /* UA2005 allclean */

                            case 3: /* UA2005 otherw */

                            case 4: /* UA2005 normalw */

                            case 5: /* UA2005 invalw */

                                // XXX

                            default:

                                goto illegal_insn;

                            }

#else

                            tcg_gen_xor_tl(cpu_tmp0, cpu_src1, cpu_src2);

                            gen_helper_wrpsr(cpu_env, cpu_tmp0);

                            tcg_gen_movi_i32(cpu_cc_op, CC_OP_FLAGS);

                            dc->cc_op = CC_OP_FLAGS;

                            save_state(dc);

                            gen_op_next_insn();

                            tcg_gen_exit_tb(0);

                            dc->is_br = 1;

#endif

                        }

                        break;

                    case 0x32: /* wrwim, V9 wrpr */

                        {

                            if (!supervisor(dc))

                                goto priv_insn;

                            tcg_gen_xor_tl(cpu_tmp0, cpu_src1, cpu_src2);

#ifdef TARGET_SPARC64

                            switch (rd) {

                            case 0: // tpc

                                {

                                    TCGv_ptr r_tsptr;



                                    r_tsptr = tcg_temp_new_ptr();

                                    gen_load_trap_state_at_tl(r_tsptr, cpu_env);

                                    tcg_gen_st_tl(cpu_tmp0, r_tsptr,

                                                  offsetof(trap_state, tpc));

                                    tcg_temp_free_ptr(r_tsptr);

                                }

                                break;

                            case 1: // tnpc

                                {

                                    TCGv_ptr r_tsptr;



                                    r_tsptr = tcg_temp_new_ptr();

                                    gen_load_trap_state_at_tl(r_tsptr, cpu_env);

                                    tcg_gen_st_tl(cpu_tmp0, r_tsptr,

                                                  offsetof(trap_state, tnpc));

                                    tcg_temp_free_ptr(r_tsptr);

                                }

                                break;

                            case 2: // tstate

                                {

                                    TCGv_ptr r_tsptr;



                                    r_tsptr = tcg_temp_new_ptr();

                                    gen_load_trap_state_at_tl(r_tsptr, cpu_env);

                                    tcg_gen_st_tl(cpu_tmp0, r_tsptr,

                                                  offsetof(trap_state,

                                                           tstate));

                                    tcg_temp_free_ptr(r_tsptr);

                                }

                                break;

                            case 3: // tt

                                {

                                    TCGv_ptr r_tsptr;



                                    r_tsptr = tcg_temp_new_ptr();

                                    gen_load_trap_state_at_tl(r_tsptr, cpu_env);

                                    tcg_gen_st32_tl(cpu_tmp0, r_tsptr,

                                                    offsetof(trap_state, tt));

                                    tcg_temp_free_ptr(r_tsptr);

                                }

                                break;

                            case 4: // tick

                                {

                                    TCGv_ptr r_tickptr;



                                    r_tickptr = tcg_temp_new_ptr();

                                    tcg_gen_ld_ptr(r_tickptr, cpu_env,

                                                   offsetof(CPUSPARCState, tick));

                                    gen_helper_tick_set_count(r_tickptr,

                                                              cpu_tmp0);

                                    tcg_temp_free_ptr(r_tickptr);

                                }

                                break;

                            case 5: // tba

                                tcg_gen_mov_tl(cpu_tbr, cpu_tmp0);

                                break;

                            case 6: // pstate

                                save_state(dc);

                                gen_helper_wrpstate(cpu_env, cpu_tmp0);

                                dc->npc = DYNAMIC_PC;

                                break;

                            case 7: // tl

                                save_state(dc);

                                tcg_gen_st32_tl(cpu_tmp0, cpu_env,

                                               offsetof(CPUSPARCState, tl));

                                dc->npc = DYNAMIC_PC;

                                break;

                            case 8: // pil

                                gen_helper_wrpil(cpu_env, cpu_tmp0);

                                break;

                            case 9: // cwp

                                gen_helper_wrcwp(cpu_env, cpu_tmp0);

                                break;

                            case 10: // cansave

                                tcg_gen_st32_tl(cpu_tmp0, cpu_env,

                                                offsetof(CPUSPARCState,

                                                         cansave));

                                break;

                            case 11: // canrestore

                                tcg_gen_st32_tl(cpu_tmp0, cpu_env,

                                                offsetof(CPUSPARCState,

                                                         canrestore));

                                break;

                            case 12: // cleanwin

                                tcg_gen_st32_tl(cpu_tmp0, cpu_env,

                                                offsetof(CPUSPARCState,

                                                         cleanwin));

                                break;

                            case 13: // otherwin

                                tcg_gen_st32_tl(cpu_tmp0, cpu_env,

                                                offsetof(CPUSPARCState,

                                                         otherwin));

                                break;

                            case 14: // wstate

                                tcg_gen_st32_tl(cpu_tmp0, cpu_env,

                                                offsetof(CPUSPARCState,

                                                         wstate));

                                break;

                            case 16: // UA2005 gl

                                CHECK_IU_FEATURE(dc, GL);

                                tcg_gen_st32_tl(cpu_tmp0, cpu_env,

                                                offsetof(CPUSPARCState, gl));

                                break;

                            case 26: // UA2005 strand status

                                CHECK_IU_FEATURE(dc, HYPV);

                                if (!hypervisor(dc))

                                    goto priv_insn;

                                tcg_gen_mov_tl(cpu_ssr, cpu_tmp0);

                                break;

                            default:

                                goto illegal_insn;

                            }

#else

                            tcg_gen_trunc_tl_i32(cpu_wim, cpu_tmp0);

                            if (dc->def->nwindows != 32) {

                                tcg_gen_andi_tl(cpu_wim, cpu_wim,

                                                (1 << dc->def->nwindows) - 1);

                            }

#endif

                        }

                        break;

                    case 0x33: /* wrtbr, UA2005 wrhpr */

                        {

#ifndef TARGET_SPARC64

                            if (!supervisor(dc))

                                goto priv_insn;

                            tcg_gen_xor_tl(cpu_tbr, cpu_src1, cpu_src2);

#else

                            CHECK_IU_FEATURE(dc, HYPV);

                            if (!hypervisor(dc))

                                goto priv_insn;

                            tcg_gen_xor_tl(cpu_tmp0, cpu_src1, cpu_src2);

                            switch (rd) {

                            case 0: // hpstate

                                // XXX gen_op_wrhpstate();

                                save_state(dc);

                                gen_op_next_insn();

                                tcg_gen_exit_tb(0);

                                dc->is_br = 1;

                                break;

                            case 1: // htstate

                                // XXX gen_op_wrhtstate();

                                break;

                            case 3: // hintp

                                tcg_gen_mov_tl(cpu_hintp, cpu_tmp0);

                                break;

                            case 5: // htba

                                tcg_gen_mov_tl(cpu_htba, cpu_tmp0);

                                break;

                            case 31: // hstick_cmpr

                                {

                                    TCGv_ptr r_tickptr;



                                    tcg_gen_mov_tl(cpu_hstick_cmpr, cpu_tmp0);

                                    r_tickptr = tcg_temp_new_ptr();

                                    tcg_gen_ld_ptr(r_tickptr, cpu_env,

                                                   offsetof(CPUSPARCState, hstick));

                                    gen_helper_tick_set_limit(r_tickptr,

                                                              cpu_hstick_cmpr);

                                    tcg_temp_free_ptr(r_tickptr);

                                }

                                break;

                            case 6: // hver readonly

                            default:

                                goto illegal_insn;

                            }

#endif

                        }

                        break;

#endif

#ifdef TARGET_SPARC64

                    case 0x2c: /* V9 movcc */

                        {

                            int cc = GET_FIELD_SP(insn, 11, 12);

                            int cond = GET_FIELD_SP(insn, 14, 17);

                            DisasCompare cmp;

                            TCGv dst;



                            if (insn & (1 << 18)) {

                                if (cc == 0) {

                                    gen_compare(&cmp, 0, cond, dc);

                                } else if (cc == 2) {

                                    gen_compare(&cmp, 1, cond, dc);

                                } else {

                                    goto illegal_insn;

                                }

                            } else {

                                gen_fcompare(&cmp, cc, cond);

                            }



                            /* The get_src2 above loaded the normal 13-bit

                               immediate field, not the 11-bit field we have

                               in movcc.  But it did handle the reg case.  */

                            if (IS_IMM) {

                                simm = GET_FIELD_SPs(insn, 0, 10);

                                tcg_gen_movi_tl(cpu_src2, simm);

                            }



                            dst = gen_load_gpr(dc, rd);

                            tcg_gen_movcond_tl(cmp.cond, dst,

                                               cmp.c1, cmp.c2,

                                               cpu_src2, dst);

                            free_compare(&cmp);

                            gen_store_gpr(dc, rd, dst);

                            break;

                        }

                    case 0x2d: /* V9 sdivx */

                        gen_helper_sdivx(cpu_dst, cpu_env, cpu_src1, cpu_src2);

                        gen_store_gpr(dc, rd, cpu_dst);

                        break;

                    case 0x2e: /* V9 popc */

                        gen_helper_popc(cpu_dst, cpu_src2);

                        gen_store_gpr(dc, rd, cpu_dst);

                        break;

                    case 0x2f: /* V9 movr */

                        {

                            int cond = GET_FIELD_SP(insn, 10, 12);

                            DisasCompare cmp;

                            TCGv dst;



                            gen_compare_reg(&cmp, cond, cpu_src1);



                            /* The get_src2 above loaded the normal 13-bit

                               immediate field, not the 10-bit field we have

                               in movr.  But it did handle the reg case.  */

                            if (IS_IMM) {

                                simm = GET_FIELD_SPs(insn, 0, 9);

                                tcg_gen_movi_tl(cpu_src2, simm);

                            }



                            dst = gen_load_gpr(dc, rd);

                            tcg_gen_movcond_tl(cmp.cond, dst,

                                               cmp.c1, cmp.c2,

                                               cpu_src2, dst);

                            free_compare(&cmp);

                            gen_store_gpr(dc, rd, dst);

                            break;

                        }

#endif

                    default:

                        goto illegal_insn;

                    }

                }

            } else if (xop == 0x36) { /* UltraSparc shutdown, VIS, V8 CPop1 */

#ifdef TARGET_SPARC64

                int opf = GET_FIELD_SP(insn, 5, 13);

                rs1 = GET_FIELD(insn, 13, 17);

                rs2 = GET_FIELD(insn, 27, 31);

                if (gen_trap_ifnofpu(dc)) {

                    goto jmp_insn;

                }



                switch (opf) {

                case 0x000: /* VIS I edge8cc */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_edge(dc, cpu_dst, cpu_src1, cpu_src2, 8, 1, 0);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x001: /* VIS II edge8n */

                    CHECK_FPU_FEATURE(dc, VIS2);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_edge(dc, cpu_dst, cpu_src1, cpu_src2, 8, 0, 0);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x002: /* VIS I edge8lcc */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_edge(dc, cpu_dst, cpu_src1, cpu_src2, 8, 1, 1);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x003: /* VIS II edge8ln */

                    CHECK_FPU_FEATURE(dc, VIS2);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_edge(dc, cpu_dst, cpu_src1, cpu_src2, 8, 0, 1);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x004: /* VIS I edge16cc */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_edge(dc, cpu_dst, cpu_src1, cpu_src2, 16, 1, 0);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x005: /* VIS II edge16n */

                    CHECK_FPU_FEATURE(dc, VIS2);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_edge(dc, cpu_dst, cpu_src1, cpu_src2, 16, 0, 0);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x006: /* VIS I edge16lcc */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_edge(dc, cpu_dst, cpu_src1, cpu_src2, 16, 1, 1);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x007: /* VIS II edge16ln */

                    CHECK_FPU_FEATURE(dc, VIS2);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_edge(dc, cpu_dst, cpu_src1, cpu_src2, 16, 0, 1);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x008: /* VIS I edge32cc */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_edge(dc, cpu_dst, cpu_src1, cpu_src2, 32, 1, 0);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x009: /* VIS II edge32n */

                    CHECK_FPU_FEATURE(dc, VIS2);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_edge(dc, cpu_dst, cpu_src1, cpu_src2, 32, 0, 0);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x00a: /* VIS I edge32lcc */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_edge(dc, cpu_dst, cpu_src1, cpu_src2, 32, 1, 1);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x00b: /* VIS II edge32ln */

                    CHECK_FPU_FEATURE(dc, VIS2);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_edge(dc, cpu_dst, cpu_src1, cpu_src2, 32, 0, 1);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x010: /* VIS I array8 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_helper_array8(cpu_dst, cpu_src1, cpu_src2);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x012: /* VIS I array16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_helper_array8(cpu_dst, cpu_src1, cpu_src2);

                    tcg_gen_shli_i64(cpu_dst, cpu_dst, 1);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x014: /* VIS I array32 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_helper_array8(cpu_dst, cpu_src1, cpu_src2);

                    tcg_gen_shli_i64(cpu_dst, cpu_dst, 2);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x018: /* VIS I alignaddr */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_alignaddr(cpu_dst, cpu_src1, cpu_src2, 0);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x01a: /* VIS I alignaddrl */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_alignaddr(cpu_dst, cpu_src1, cpu_src2, 1);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x019: /* VIS II bmask */

                    CHECK_FPU_FEATURE(dc, VIS2);

                    cpu_src1 = gen_load_gpr(dc, rs1);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    tcg_gen_add_tl(cpu_dst, cpu_src1, cpu_src2);

                    tcg_gen_deposit_tl(cpu_gsr, cpu_gsr, cpu_dst, 32, 32);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x020: /* VIS I fcmple16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_64 = gen_load_fpr_D(dc, rs1);

                    cpu_src2_64 = gen_load_fpr_D(dc, rs2);

                    gen_helper_fcmple16(cpu_dst, cpu_src1_64, cpu_src2_64);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x022: /* VIS I fcmpne16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_64 = gen_load_fpr_D(dc, rs1);

                    cpu_src2_64 = gen_load_fpr_D(dc, rs2);

                    gen_helper_fcmpne16(cpu_dst, cpu_src1_64, cpu_src2_64);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x024: /* VIS I fcmple32 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_64 = gen_load_fpr_D(dc, rs1);

                    cpu_src2_64 = gen_load_fpr_D(dc, rs2);

                    gen_helper_fcmple32(cpu_dst, cpu_src1_64, cpu_src2_64);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x026: /* VIS I fcmpne32 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_64 = gen_load_fpr_D(dc, rs1);

                    cpu_src2_64 = gen_load_fpr_D(dc, rs2);

                    gen_helper_fcmpne32(cpu_dst, cpu_src1_64, cpu_src2_64);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x028: /* VIS I fcmpgt16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_64 = gen_load_fpr_D(dc, rs1);

                    cpu_src2_64 = gen_load_fpr_D(dc, rs2);

                    gen_helper_fcmpgt16(cpu_dst, cpu_src1_64, cpu_src2_64);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x02a: /* VIS I fcmpeq16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_64 = gen_load_fpr_D(dc, rs1);

                    cpu_src2_64 = gen_load_fpr_D(dc, rs2);

                    gen_helper_fcmpeq16(cpu_dst, cpu_src1_64, cpu_src2_64);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x02c: /* VIS I fcmpgt32 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_64 = gen_load_fpr_D(dc, rs1);

                    cpu_src2_64 = gen_load_fpr_D(dc, rs2);

                    gen_helper_fcmpgt32(cpu_dst, cpu_src1_64, cpu_src2_64);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x02e: /* VIS I fcmpeq32 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_64 = gen_load_fpr_D(dc, rs1);

                    cpu_src2_64 = gen_load_fpr_D(dc, rs2);

                    gen_helper_fcmpeq32(cpu_dst, cpu_src1_64, cpu_src2_64);

                    gen_store_gpr(dc, rd, cpu_dst);

                    break;

                case 0x031: /* VIS I fmul8x16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fmul8x16);

                    break;

                case 0x033: /* VIS I fmul8x16au */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fmul8x16au);

                    break;

                case 0x035: /* VIS I fmul8x16al */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fmul8x16al);

                    break;

                case 0x036: /* VIS I fmul8sux16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fmul8sux16);

                    break;

                case 0x037: /* VIS I fmul8ulx16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fmul8ulx16);

                    break;

                case 0x038: /* VIS I fmuld8sux16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fmuld8sux16);

                    break;

                case 0x039: /* VIS I fmuld8ulx16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fmuld8ulx16);

                    break;

                case 0x03a: /* VIS I fpack32 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_gsr_fop_DDD(dc, rd, rs1, rs2, gen_helper_fpack32);

                    break;

                case 0x03b: /* VIS I fpack16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_64 = gen_load_fpr_D(dc, rs2);

                    cpu_dst_32 = gen_dest_fpr_F(dc);

                    gen_helper_fpack16(cpu_dst_32, cpu_gsr, cpu_src1_64);

                    gen_store_fpr_F(dc, rd, cpu_dst_32);

                    break;

                case 0x03d: /* VIS I fpackfix */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_64 = gen_load_fpr_D(dc, rs2);

                    cpu_dst_32 = gen_dest_fpr_F(dc);

                    gen_helper_fpackfix(cpu_dst_32, cpu_gsr, cpu_src1_64);

                    gen_store_fpr_F(dc, rd, cpu_dst_32);

                    break;

                case 0x03e: /* VIS I pdist */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDDD(dc, rd, rs1, rs2, gen_helper_pdist);

                    break;

                case 0x048: /* VIS I faligndata */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_gsr_fop_DDD(dc, rd, rs1, rs2, gen_faligndata);

                    break;

                case 0x04b: /* VIS I fpmerge */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fpmerge);

                    break;

                case 0x04c: /* VIS II bshuffle */

                    CHECK_FPU_FEATURE(dc, VIS2);

                    gen_gsr_fop_DDD(dc, rd, rs1, rs2, gen_helper_bshuffle);

                    break;

                case 0x04d: /* VIS I fexpand */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fexpand);

                    break;

                case 0x050: /* VIS I fpadd16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fpadd16);

                    break;

                case 0x051: /* VIS I fpadd16s */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs1, rs2, gen_helper_fpadd16s);

                    break;

                case 0x052: /* VIS I fpadd32 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fpadd32);

                    break;

                case 0x053: /* VIS I fpadd32s */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs1, rs2, tcg_gen_add_i32);

                    break;

                case 0x054: /* VIS I fpsub16 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fpsub16);

                    break;

                case 0x055: /* VIS I fpsub16s */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs1, rs2, gen_helper_fpsub16s);

                    break;

                case 0x056: /* VIS I fpsub32 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, gen_helper_fpsub32);

                    break;

                case 0x057: /* VIS I fpsub32s */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs1, rs2, tcg_gen_sub_i32);

                    break;

                case 0x060: /* VIS I fzero */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_dst_64 = gen_dest_fpr_D(dc, rd);

                    tcg_gen_movi_i64(cpu_dst_64, 0);

                    gen_store_fpr_D(dc, rd, cpu_dst_64);

                    break;

                case 0x061: /* VIS I fzeros */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_dst_32 = gen_dest_fpr_F(dc);

                    tcg_gen_movi_i32(cpu_dst_32, 0);

                    gen_store_fpr_F(dc, rd, cpu_dst_32);

                    break;

                case 0x062: /* VIS I fnor */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, tcg_gen_nor_i64);

                    break;

                case 0x063: /* VIS I fnors */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs1, rs2, tcg_gen_nor_i32);

                    break;

                case 0x064: /* VIS I fandnot2 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, tcg_gen_andc_i64);

                    break;

                case 0x065: /* VIS I fandnot2s */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs1, rs2, tcg_gen_andc_i32);

                    break;

                case 0x066: /* VIS I fnot2 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DD(dc, rd, rs2, tcg_gen_not_i64);

                    break;

                case 0x067: /* VIS I fnot2s */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FF(dc, rd, rs2, tcg_gen_not_i32);

                    break;

                case 0x068: /* VIS I fandnot1 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs2, rs1, tcg_gen_andc_i64);

                    break;

                case 0x069: /* VIS I fandnot1s */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs2, rs1, tcg_gen_andc_i32);

                    break;

                case 0x06a: /* VIS I fnot1 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DD(dc, rd, rs1, tcg_gen_not_i64);

                    break;

                case 0x06b: /* VIS I fnot1s */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FF(dc, rd, rs1, tcg_gen_not_i32);

                    break;

                case 0x06c: /* VIS I fxor */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, tcg_gen_xor_i64);

                    break;

                case 0x06d: /* VIS I fxors */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs1, rs2, tcg_gen_xor_i32);

                    break;

                case 0x06e: /* VIS I fnand */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, tcg_gen_nand_i64);

                    break;

                case 0x06f: /* VIS I fnands */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs1, rs2, tcg_gen_nand_i32);

                    break;

                case 0x070: /* VIS I fand */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, tcg_gen_and_i64);

                    break;

                case 0x071: /* VIS I fands */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs1, rs2, tcg_gen_and_i32);

                    break;

                case 0x072: /* VIS I fxnor */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, tcg_gen_eqv_i64);

                    break;

                case 0x073: /* VIS I fxnors */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs1, rs2, tcg_gen_eqv_i32);

                    break;

                case 0x074: /* VIS I fsrc1 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_64 = gen_load_fpr_D(dc, rs1);

                    gen_store_fpr_D(dc, rd, cpu_src1_64);

                    break;

                case 0x075: /* VIS I fsrc1s */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_32 = gen_load_fpr_F(dc, rs1);

                    gen_store_fpr_F(dc, rd, cpu_src1_32);

                    break;

                case 0x076: /* VIS I fornot2 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, tcg_gen_orc_i64);

                    break;

                case 0x077: /* VIS I fornot2s */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs1, rs2, tcg_gen_orc_i32);

                    break;

                case 0x078: /* VIS I fsrc2 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_64 = gen_load_fpr_D(dc, rs2);

                    gen_store_fpr_D(dc, rd, cpu_src1_64);

                    break;

                case 0x079: /* VIS I fsrc2s */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_src1_32 = gen_load_fpr_F(dc, rs2);

                    gen_store_fpr_F(dc, rd, cpu_src1_32);

                    break;

                case 0x07a: /* VIS I fornot1 */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs2, rs1, tcg_gen_orc_i64);

                    break;

                case 0x07b: /* VIS I fornot1s */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs2, rs1, tcg_gen_orc_i32);

                    break;

                case 0x07c: /* VIS I for */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_DDD(dc, rd, rs1, rs2, tcg_gen_or_i64);

                    break;

                case 0x07d: /* VIS I fors */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    gen_ne_fop_FFF(dc, rd, rs1, rs2, tcg_gen_or_i32);

                    break;

                case 0x07e: /* VIS I fone */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_dst_64 = gen_dest_fpr_D(dc, rd);

                    tcg_gen_movi_i64(cpu_dst_64, -1);

                    gen_store_fpr_D(dc, rd, cpu_dst_64);

                    break;

                case 0x07f: /* VIS I fones */

                    CHECK_FPU_FEATURE(dc, VIS1);

                    cpu_dst_32 = gen_dest_fpr_F(dc);

                    tcg_gen_movi_i32(cpu_dst_32, -1);

                    gen_store_fpr_F(dc, rd, cpu_dst_32);

                    break;

                case 0x080: /* VIS I shutdown */

                case 0x081: /* VIS II siam */

                    // XXX

                    goto illegal_insn;

                default:

                    goto illegal_insn;

                }

#else

                goto ncp_insn;

#endif

            } else if (xop == 0x37) { /* V8 CPop2, V9 impdep2 */

#ifdef TARGET_SPARC64

                goto illegal_insn;

#else

                goto ncp_insn;

#endif

#ifdef TARGET_SPARC64

            } else if (xop == 0x39) { /* V9 return */

                TCGv_i32 r_const;



                save_state(dc);

                cpu_src1 = get_src1(dc, insn);

                if (IS_IMM) {   /* immediate */

                    simm = GET_FIELDs(insn, 19, 31);

                    tcg_gen_addi_tl(cpu_tmp0, cpu_src1, simm);

                } else {                /* register */

                    rs2 = GET_FIELD(insn, 27, 31);

                    if (rs2) {

                        cpu_src2 = gen_load_gpr(dc, rs2);

                        tcg_gen_add_tl(cpu_tmp0, cpu_src1, cpu_src2);

                    } else {

                        tcg_gen_mov_tl(cpu_tmp0, cpu_src1);

                    }

                }

                gen_helper_restore(cpu_env);

                gen_mov_pc_npc(dc);

                r_const = tcg_const_i32(3);

                gen_helper_check_align(cpu_env, cpu_tmp0, r_const);

                tcg_temp_free_i32(r_const);

                tcg_gen_mov_tl(cpu_npc, cpu_tmp0);

                dc->npc = DYNAMIC_PC;

                goto jmp_insn;

#endif

            } else {

                cpu_src1 = get_src1(dc, insn);

                if (IS_IMM) {   /* immediate */

                    simm = GET_FIELDs(insn, 19, 31);

                    tcg_gen_addi_tl(cpu_tmp0, cpu_src1, simm);

                } else {                /* register */

                    rs2 = GET_FIELD(insn, 27, 31);

                    if (rs2) {

                        cpu_src2 = gen_load_gpr(dc, rs2);

                        tcg_gen_add_tl(cpu_tmp0, cpu_src1, cpu_src2);

                    } else {

                        tcg_gen_mov_tl(cpu_tmp0, cpu_src1);

                    }

                }

                switch (xop) {

                case 0x38:      /* jmpl */

                    {

                        TCGv t;

                        TCGv_i32 r_const;



                        t = gen_dest_gpr(dc, rd);

                        tcg_gen_movi_tl(t, dc->pc);

                        gen_store_gpr(dc, rd, t);

                        gen_mov_pc_npc(dc);

                        r_const = tcg_const_i32(3);

                        gen_helper_check_align(cpu_env, cpu_tmp0, r_const);

                        tcg_temp_free_i32(r_const);

                        gen_address_mask(dc, cpu_tmp0);

                        tcg_gen_mov_tl(cpu_npc, cpu_tmp0);

                        dc->npc = DYNAMIC_PC;

                    }

                    goto jmp_insn;

#if !defined(CONFIG_USER_ONLY) && !defined(TARGET_SPARC64)

                case 0x39:      /* rett, V9 return */

                    {

                        TCGv_i32 r_const;



                        if (!supervisor(dc))

                            goto priv_insn;

                        gen_mov_pc_npc(dc);

                        r_const = tcg_const_i32(3);

                        gen_helper_check_align(cpu_env, cpu_tmp0, r_const);

                        tcg_temp_free_i32(r_const);

                        tcg_gen_mov_tl(cpu_npc, cpu_tmp0);

                        dc->npc = DYNAMIC_PC;

                        gen_helper_rett(cpu_env);

                    }

                    goto jmp_insn;

#endif

                case 0x3b: /* flush */

                    if (!((dc)->def->features & CPU_FEATURE_FLUSH))

                        goto unimp_flush;

                    /* nop */

                    break;

                case 0x3c:      /* save */

                    save_state(dc);

                    gen_helper_save(cpu_env);

                    gen_store_gpr(dc, rd, cpu_tmp0);

                    break;

                case 0x3d:      /* restore */

                    save_state(dc);

                    gen_helper_restore(cpu_env);

                    gen_store_gpr(dc, rd, cpu_tmp0);

                    break;

#if !defined(CONFIG_USER_ONLY) && defined(TARGET_SPARC64)

                case 0x3e:      /* V9 done/retry */

                    {

                        switch (rd) {

                        case 0:

                            if (!supervisor(dc))

                                goto priv_insn;

                            dc->npc = DYNAMIC_PC;

                            dc->pc = DYNAMIC_PC;

                            gen_helper_done(cpu_env);

                            goto jmp_insn;

                        case 1:

                            if (!supervisor(dc))

                                goto priv_insn;

                            dc->npc = DYNAMIC_PC;

                            dc->pc = DYNAMIC_PC;

                            gen_helper_retry(cpu_env);

                            goto jmp_insn;

                        default:

                            goto illegal_insn;

                        }

                    }

                    break;

#endif

                default:

                    goto illegal_insn;

                }

            }

            break;

        }

        break;

    case 3:                     /* load/store instructions */

        {

            unsigned int xop = GET_FIELD(insn, 7, 12);

            /* ??? gen_address_mask prevents us from using a source

               register directly.  Always generate a temporary.  */

            TCGv cpu_addr = get_temp_tl(dc);



            tcg_gen_mov_tl(cpu_addr, get_src1(dc, insn));

            if (xop == 0x3c || xop == 0x3e) {

                /* V9 casa/casxa : no offset */

            } else if (IS_IMM) {     /* immediate */

                simm = GET_FIELDs(insn, 19, 31);

                if (simm != 0) {

                    tcg_gen_addi_tl(cpu_addr, cpu_addr, simm);

                }

            } else {            /* register */

                rs2 = GET_FIELD(insn, 27, 31);

                if (rs2 != 0) {

                    tcg_gen_add_tl(cpu_addr, cpu_addr, gen_load_gpr(dc, rs2));

                }

            }

            if (xop < 4 || (xop > 7 && xop < 0x14 && xop != 0x0e) ||

                (xop > 0x17 && xop <= 0x1d ) ||

                (xop > 0x2c && xop <= 0x33) || xop == 0x1f || xop == 0x3d) {

                TCGv cpu_val = gen_dest_gpr(dc, rd);



                switch (xop) {

                case 0x0:       /* ld, V9 lduw, load unsigned word */

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_ld32u(cpu_val, cpu_addr, dc->mem_idx);

                    break;

                case 0x1:       /* ldub, load unsigned byte */

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_ld8u(cpu_val, cpu_addr, dc->mem_idx);

                    break;

                case 0x2:       /* lduh, load unsigned halfword */

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_ld16u(cpu_val, cpu_addr, dc->mem_idx);

                    break;

                case 0x3:       /* ldd, load double word */

                    if (rd & 1)

                        goto illegal_insn;

                    else {

                        TCGv_i32 r_const;

                        TCGv_i64 t64;



                        save_state(dc);

                        r_const = tcg_const_i32(7);

                        /* XXX remove alignment check */

                        gen_helper_check_align(cpu_env, cpu_addr, r_const);

                        tcg_temp_free_i32(r_const);

                        gen_address_mask(dc, cpu_addr);

                        t64 = tcg_temp_new_i64();

                        tcg_gen_qemu_ld64(t64, cpu_addr, dc->mem_idx);

                        tcg_gen_trunc_i64_tl(cpu_tmp0, t64);

                        tcg_gen_andi_tl(cpu_tmp0, cpu_tmp0, 0xffffffffULL);

                        gen_store_gpr(dc, rd + 1, cpu_tmp0);

                        tcg_gen_shri_i64(t64, t64, 32);

                        tcg_gen_trunc_i64_tl(cpu_val, t64);

                        tcg_temp_free_i64(t64);

                        tcg_gen_andi_tl(cpu_val, cpu_val, 0xffffffffULL);

                    }

                    break;

                case 0x9:       /* ldsb, load signed byte */

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_ld8s(cpu_val, cpu_addr, dc->mem_idx);

                    break;

                case 0xa:       /* ldsh, load signed halfword */

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_ld16s(cpu_val, cpu_addr, dc->mem_idx);

                    break;

                case 0xd:       /* ldstub -- XXX: should be atomically */

                    {

                        TCGv r_const;



                        gen_address_mask(dc, cpu_addr);

                        tcg_gen_qemu_ld8s(cpu_val, cpu_addr, dc->mem_idx);

                        r_const = tcg_const_tl(0xff);

                        tcg_gen_qemu_st8(r_const, cpu_addr, dc->mem_idx);

                        tcg_temp_free(r_const);

                    }

                    break;

                case 0x0f:      /* swap, swap register with memory. Also

                                   atomically */

                    CHECK_IU_FEATURE(dc, SWAP);

                    cpu_src1 = gen_load_gpr(dc, rd);

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_ld32u(cpu_tmp0, cpu_addr, dc->mem_idx);

                    tcg_gen_qemu_st32(cpu_src1, cpu_addr, dc->mem_idx);

                    tcg_gen_mov_tl(cpu_val, cpu_tmp0);

                    break;

#if !defined(CONFIG_USER_ONLY) || defined(TARGET_SPARC64)

                case 0x10:      /* lda, V9 lduwa, load word alternate */

#ifndef TARGET_SPARC64

                    if (IS_IMM)

                        goto illegal_insn;

                    if (!supervisor(dc))

                        goto priv_insn;

#endif

                    save_state(dc);

                    gen_ld_asi(cpu_val, cpu_addr, insn, 4, 0);

                    break;

                case 0x11:      /* lduba, load unsigned byte alternate */

#ifndef TARGET_SPARC64

                    if (IS_IMM)

                        goto illegal_insn;

                    if (!supervisor(dc))

                        goto priv_insn;

#endif

                    save_state(dc);

                    gen_ld_asi(cpu_val, cpu_addr, insn, 1, 0);

                    break;

                case 0x12:      /* lduha, load unsigned halfword alternate */

#ifndef TARGET_SPARC64

                    if (IS_IMM)

                        goto illegal_insn;

                    if (!supervisor(dc))

                        goto priv_insn;

#endif

                    save_state(dc);

                    gen_ld_asi(cpu_val, cpu_addr, insn, 2, 0);

                    break;

                case 0x13:      /* ldda, load double word alternate */

#ifndef TARGET_SPARC64

                    if (IS_IMM)

                        goto illegal_insn;

                    if (!supervisor(dc))

                        goto priv_insn;

#endif

                    if (rd & 1)

                        goto illegal_insn;

                    save_state(dc);

                    gen_ldda_asi(dc, cpu_val, cpu_addr, insn, rd);

                    goto skip_move;

                case 0x19:      /* ldsba, load signed byte alternate */

#ifndef TARGET_SPARC64

                    if (IS_IMM)

                        goto illegal_insn;

                    if (!supervisor(dc))

                        goto priv_insn;

#endif

                    save_state(dc);

                    gen_ld_asi(cpu_val, cpu_addr, insn, 1, 1);

                    break;

                case 0x1a:      /* ldsha, load signed halfword alternate */

#ifndef TARGET_SPARC64

                    if (IS_IMM)

                        goto illegal_insn;

                    if (!supervisor(dc))

                        goto priv_insn;

#endif

                    save_state(dc);

                    gen_ld_asi(cpu_val, cpu_addr, insn, 2, 1);

                    break;

                case 0x1d:      /* ldstuba -- XXX: should be atomically */

#ifndef TARGET_SPARC64

                    if (IS_IMM)

                        goto illegal_insn;

                    if (!supervisor(dc))

                        goto priv_insn;

#endif

                    save_state(dc);

                    gen_ldstub_asi(cpu_val, cpu_addr, insn);

                    break;

                case 0x1f:      /* swapa, swap reg with alt. memory. Also

                                   atomically */

                    CHECK_IU_FEATURE(dc, SWAP);

#ifndef TARGET_SPARC64

                    if (IS_IMM)

                        goto illegal_insn;

                    if (!supervisor(dc))

                        goto priv_insn;

#endif

                    save_state(dc);

                    cpu_src1 = gen_load_gpr(dc, rd);

                    gen_swap_asi(cpu_val, cpu_src1, cpu_addr, insn);

                    break;



#ifndef TARGET_SPARC64

                case 0x30: /* ldc */

                case 0x31: /* ldcsr */

                case 0x33: /* lddc */

                    goto ncp_insn;

#endif

#endif

#ifdef TARGET_SPARC64

                case 0x08: /* V9 ldsw */

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_ld32s(cpu_val, cpu_addr, dc->mem_idx);

                    break;

                case 0x0b: /* V9 ldx */

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_ld64(cpu_val, cpu_addr, dc->mem_idx);

                    break;

                case 0x18: /* V9 ldswa */

                    save_state(dc);

                    gen_ld_asi(cpu_val, cpu_addr, insn, 4, 1);

                    break;

                case 0x1b: /* V9 ldxa */

                    save_state(dc);

                    gen_ld_asi(cpu_val, cpu_addr, insn, 8, 0);

                    break;

                case 0x2d: /* V9 prefetch, no effect */

                    goto skip_move;

                case 0x30: /* V9 ldfa */

                    if (gen_trap_ifnofpu(dc)) {

                        goto jmp_insn;

                    }

                    save_state(dc);

                    gen_ldf_asi(cpu_addr, insn, 4, rd);

                    gen_update_fprs_dirty(rd);

                    goto skip_move;

                case 0x33: /* V9 lddfa */

                    if (gen_trap_ifnofpu(dc)) {

                        goto jmp_insn;

                    }

                    save_state(dc);

                    gen_ldf_asi(cpu_addr, insn, 8, DFPREG(rd));

                    gen_update_fprs_dirty(DFPREG(rd));

                    goto skip_move;

                case 0x3d: /* V9 prefetcha, no effect */

                    goto skip_move;

                case 0x32: /* V9 ldqfa */

                    CHECK_FPU_FEATURE(dc, FLOAT128);

                    if (gen_trap_ifnofpu(dc)) {

                        goto jmp_insn;

                    }

                    save_state(dc);

                    gen_ldf_asi(cpu_addr, insn, 16, QFPREG(rd));

                    gen_update_fprs_dirty(QFPREG(rd));

                    goto skip_move;

#endif

                default:

                    goto illegal_insn;

                }

                gen_store_gpr(dc, rd, cpu_val);

#if !defined(CONFIG_USER_ONLY) || defined(TARGET_SPARC64)

            skip_move: ;

#endif

            } else if (xop >= 0x20 && xop < 0x24) {

                if (gen_trap_ifnofpu(dc)) {

                    goto jmp_insn;

                }

                save_state(dc);

                switch (xop) {

                case 0x20:      /* ldf, load fpreg */

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_ld32u(cpu_tmp0, cpu_addr, dc->mem_idx);

                    cpu_dst_32 = gen_dest_fpr_F(dc);

                    tcg_gen_trunc_tl_i32(cpu_dst_32, cpu_tmp0);

                    gen_store_fpr_F(dc, rd, cpu_dst_32);

                    break;

                case 0x21:      /* ldfsr, V9 ldxfsr */

#ifdef TARGET_SPARC64

                    gen_address_mask(dc, cpu_addr);

                    if (rd == 1) {

                        TCGv_i64 t64 = tcg_temp_new_i64();

                        tcg_gen_qemu_ld64(t64, cpu_addr, dc->mem_idx);

                        gen_helper_ldxfsr(cpu_env, t64);

                        tcg_temp_free_i64(t64);

                        break;

                    }

#endif

                    {

                        TCGv_i32 t32 = get_temp_i32(dc);

                        tcg_gen_qemu_ld32u(cpu_tmp0, cpu_addr, dc->mem_idx);

                        tcg_gen_trunc_tl_i32(t32, cpu_tmp0);

                        gen_helper_ldfsr(cpu_env, t32);

                    }

                    break;

                case 0x22:      /* ldqf, load quad fpreg */

                    {

                        TCGv_i32 r_const;



                        CHECK_FPU_FEATURE(dc, FLOAT128);

                        r_const = tcg_const_i32(dc->mem_idx);

                        gen_address_mask(dc, cpu_addr);

                        gen_helper_ldqf(cpu_env, cpu_addr, r_const);

                        tcg_temp_free_i32(r_const);

                        gen_op_store_QT0_fpr(QFPREG(rd));

                        gen_update_fprs_dirty(QFPREG(rd));

                    }

                    break;

                case 0x23:      /* lddf, load double fpreg */

                    gen_address_mask(dc, cpu_addr);

                    cpu_dst_64 = gen_dest_fpr_D(dc, rd);

                    tcg_gen_qemu_ld64(cpu_dst_64, cpu_addr, dc->mem_idx);

                    gen_store_fpr_D(dc, rd, cpu_dst_64);

                    break;

                default:

                    goto illegal_insn;

                }

            } else if (xop < 8 || (xop >= 0x14 && xop < 0x18) ||

                       xop == 0xe || xop == 0x1e) {

                TCGv cpu_val = gen_load_gpr(dc, rd);



                switch (xop) {

                case 0x4: /* st, store word */

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_st32(cpu_val, cpu_addr, dc->mem_idx);

                    break;

                case 0x5: /* stb, store byte */

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_st8(cpu_val, cpu_addr, dc->mem_idx);

                    break;

                case 0x6: /* sth, store halfword */

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_st16(cpu_val, cpu_addr, dc->mem_idx);

                    break;

                case 0x7: /* std, store double word */

                    if (rd & 1)

                        goto illegal_insn;

                    else {

                        TCGv_i32 r_const;

                        TCGv_i64 t64;

                        TCGv lo;



                        save_state(dc);

                        gen_address_mask(dc, cpu_addr);

                        r_const = tcg_const_i32(7);

                        /* XXX remove alignment check */

                        gen_helper_check_align(cpu_env, cpu_addr, r_const);

                        tcg_temp_free_i32(r_const);

                        lo = gen_load_gpr(dc, rd + 1);



                        t64 = tcg_temp_new_i64();

                        tcg_gen_concat_tl_i64(t64, lo, cpu_val);

                        tcg_gen_qemu_st64(t64, cpu_addr, dc->mem_idx);

                        tcg_temp_free_i64(t64);

                    }

                    break;

#if !defined(CONFIG_USER_ONLY) || defined(TARGET_SPARC64)

                case 0x14: /* sta, V9 stwa, store word alternate */

#ifndef TARGET_SPARC64

                    if (IS_IMM)

                        goto illegal_insn;

                    if (!supervisor(dc))

                        goto priv_insn;

#endif

                    save_state(dc);

                    gen_st_asi(cpu_val, cpu_addr, insn, 4);

                    dc->npc = DYNAMIC_PC;

                    break;

                case 0x15: /* stba, store byte alternate */

#ifndef TARGET_SPARC64

                    if (IS_IMM)

                        goto illegal_insn;

                    if (!supervisor(dc))

                        goto priv_insn;

#endif

                    save_state(dc);

                    gen_st_asi(cpu_val, cpu_addr, insn, 1);

                    dc->npc = DYNAMIC_PC;

                    break;

                case 0x16: /* stha, store halfword alternate */

#ifndef TARGET_SPARC64

                    if (IS_IMM)

                        goto illegal_insn;

                    if (!supervisor(dc))

                        goto priv_insn;

#endif

                    save_state(dc);

                    gen_st_asi(cpu_val, cpu_addr, insn, 2);

                    dc->npc = DYNAMIC_PC;

                    break;

                case 0x17: /* stda, store double word alternate */

#ifndef TARGET_SPARC64

                    if (IS_IMM)

                        goto illegal_insn;

                    if (!supervisor(dc))

                        goto priv_insn;

#endif

                    if (rd & 1)

                        goto illegal_insn;

                    else {

                        save_state(dc);

                        gen_stda_asi(dc, cpu_val, cpu_addr, insn, rd);

                    }

                    break;

#endif

#ifdef TARGET_SPARC64

                case 0x0e: /* V9 stx */

                    gen_address_mask(dc, cpu_addr);

                    tcg_gen_qemu_st64(cpu_val, cpu_addr, dc->mem_idx);

                    break;

                case 0x1e: /* V9 stxa */

                    save_state(dc);

                    gen_st_asi(cpu_val, cpu_addr, insn, 8);

                    dc->npc = DYNAMIC_PC;

                    break;

#endif

                default:

                    goto illegal_insn;

                }

            } else if (xop > 0x23 && xop < 0x28) {

                if (gen_trap_ifnofpu(dc)) {

                    goto jmp_insn;

                }

                save_state(dc);

                switch (xop) {

                case 0x24: /* stf, store fpreg */

                    gen_address_mask(dc, cpu_addr);

                    cpu_src1_32 = gen_load_fpr_F(dc, rd);

                    tcg_gen_ext_i32_tl(cpu_tmp0, cpu_src1_32);

                    tcg_gen_qemu_st32(cpu_tmp0, cpu_addr, dc->mem_idx);

                    break;

                case 0x25: /* stfsr, V9 stxfsr */

                    {

                        TCGv t = get_temp_tl(dc);



                        tcg_gen_ld_tl(t, cpu_env, offsetof(CPUSPARCState, fsr));

#ifdef TARGET_SPARC64

                        gen_address_mask(dc, cpu_addr);

                        if (rd == 1) {

                            tcg_gen_qemu_st64(t, cpu_addr, dc->mem_idx);

                            break;

                        }

#endif

                        tcg_gen_qemu_st32(t, cpu_addr, dc->mem_idx);

                    }

                    break;

                case 0x26:

#ifdef TARGET_SPARC64

                    /* V9 stqf, store quad fpreg */

                    {

                        TCGv_i32 r_const;



                        CHECK_FPU_FEATURE(dc, FLOAT128);

                        gen_op_load_fpr_QT0(QFPREG(rd));

                        r_const = tcg_const_i32(dc->mem_idx);

                        gen_address_mask(dc, cpu_addr);

                        gen_helper_stqf(cpu_env, cpu_addr, r_const);

                        tcg_temp_free_i32(r_const);

                    }

                    break;

#else /* !TARGET_SPARC64 */

                    /* stdfq, store floating point queue */

#if defined(CONFIG_USER_ONLY)

                    goto illegal_insn;

#else

                    if (!supervisor(dc))

                        goto priv_insn;

                    if (gen_trap_ifnofpu(dc)) {

                        goto jmp_insn;

                    }

                    goto nfq_insn;

#endif

#endif

                case 0x27: /* stdf, store double fpreg */

                    gen_address_mask(dc, cpu_addr);

                    cpu_src1_64 = gen_load_fpr_D(dc, rd);

                    tcg_gen_qemu_st64(cpu_src1_64, cpu_addr, dc->mem_idx);

                    break;

                default:

                    goto illegal_insn;

                }

            } else if (xop > 0x33 && xop < 0x3f) {

                save_state(dc);

                switch (xop) {

#ifdef TARGET_SPARC64

                case 0x34: /* V9 stfa */

                    if (gen_trap_ifnofpu(dc)) {

                        goto jmp_insn;

                    }

                    gen_stf_asi(cpu_addr, insn, 4, rd);

                    break;

                case 0x36: /* V9 stqfa */

                    {

                        TCGv_i32 r_const;



                        CHECK_FPU_FEATURE(dc, FLOAT128);

                        if (gen_trap_ifnofpu(dc)) {

                            goto jmp_insn;

                        }

                        r_const = tcg_const_i32(7);

                        gen_helper_check_align(cpu_env, cpu_addr, r_const);

                        tcg_temp_free_i32(r_const);

                        gen_stf_asi(cpu_addr, insn, 16, QFPREG(rd));

                    }

                    break;

                case 0x37: /* V9 stdfa */

                    if (gen_trap_ifnofpu(dc)) {

                        goto jmp_insn;

                    }

                    gen_stf_asi(cpu_addr, insn, 8, DFPREG(rd));

                    break;

                case 0x3c: /* V9 casa */

                    rs2 = GET_FIELD(insn, 27, 31);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_cas_asi(dc, cpu_addr, cpu_src2, insn, rd);

                    break;

                case 0x3e: /* V9 casxa */

                    rs2 = GET_FIELD(insn, 27, 31);

                    cpu_src2 = gen_load_gpr(dc, rs2);

                    gen_casx_asi(dc, cpu_addr, cpu_src2, insn, rd);

                    break;

#else

                case 0x34: /* stc */

                case 0x35: /* stcsr */

                case 0x36: /* stdcq */

                case 0x37: /* stdc */

                    goto ncp_insn;

#endif

                default:

                    goto illegal_insn;

                }

            } else {

                goto illegal_insn;

            }

        }

        break;

    }

    /* default case for non jump instructions */

    if (dc->npc == DYNAMIC_PC) {

        dc->pc = DYNAMIC_PC;

        gen_op_next_insn();

    } else if (dc->npc == JUMP_PC) {

        /* we can do a static jump */

        gen_branch2(dc, dc->jump_pc[0], dc->jump_pc[1], cpu_cond);

        dc->is_br = 1;

    } else {

        dc->pc = dc->npc;

        dc->npc = dc->npc + 4;

    }

 jmp_insn:

    goto egress;

 illegal_insn:

    {

        TCGv_i32 r_const;



        save_state(dc);

        r_const = tcg_const_i32(TT_ILL_INSN);

        gen_helper_raise_exception(cpu_env, r_const);

        tcg_temp_free_i32(r_const);

        dc->is_br = 1;

    }

    goto egress;

 unimp_flush:

    {

        TCGv_i32 r_const;



        save_state(dc);

        r_const = tcg_const_i32(TT_UNIMP_FLUSH);

        gen_helper_raise_exception(cpu_env, r_const);

        tcg_temp_free_i32(r_const);

        dc->is_br = 1;

    }

    goto egress;

#if !defined(CONFIG_USER_ONLY)

 priv_insn:

    {

        TCGv_i32 r_const;



        save_state(dc);

        r_const = tcg_const_i32(TT_PRIV_INSN);

        gen_helper_raise_exception(cpu_env, r_const);

        tcg_temp_free_i32(r_const);

        dc->is_br = 1;

    }

    goto egress;

#endif

 nfpu_insn:

    save_state(dc);

    gen_op_fpexception_im(FSR_FTT_UNIMPFPOP);

    dc->is_br = 1;

    goto egress;

#if !defined(CONFIG_USER_ONLY) && !defined(TARGET_SPARC64)

 nfq_insn:

    save_state(dc);

    gen_op_fpexception_im(FSR_FTT_SEQ_ERROR);

    dc->is_br = 1;

    goto egress;

#endif

#ifndef TARGET_SPARC64

 ncp_insn:

    {

        TCGv r_const;



        save_state(dc);

        r_const = tcg_const_i32(TT_NCP_INSN);

        gen_helper_raise_exception(cpu_env, r_const);

        tcg_temp_free(r_const);

        dc->is_br = 1;

    }

    goto egress;

#endif

 egress:

    if (dc->n_t32 != 0) {

        int i;

        for (i = dc->n_t32 - 1; i >= 0; --i) {

            tcg_temp_free_i32(dc->t32[i]);

        }

        dc->n_t32 = 0;

    }

    if (dc->n_ttl != 0) {

        int i;

        for (i = dc->n_ttl - 1; i >= 0; --i) {

            tcg_temp_free(dc->ttl[i]);

        }

        dc->n_ttl = 0;

    }

}
