static always_inline void gen_intermediate_code_internal (CPUState *env,

                                                          TranslationBlock *tb,

                                                          int search_pc)

{

    DisasContext ctx, *ctxp = &ctx;

    opc_handler_t **table, *handler;

    target_ulong pc_start;

    uint16_t *gen_opc_end;

    CPUBreakpoint *bp;

    int j, lj = -1;

    int num_insns;

    int max_insns;



    pc_start = tb->pc;

    gen_opc_end = gen_opc_buf + OPC_MAX_SIZE;

    ctx.nip = pc_start;

    ctx.tb = tb;

    ctx.exception = POWERPC_EXCP_NONE;

    ctx.spr_cb = env->spr_cb;

    ctx.mem_idx = env->mmu_idx;

    ctx.access_type = -1;

    ctx.le_mode = env->hflags & (1 << MSR_LE) ? 1 : 0;

#if defined(TARGET_PPC64)

    ctx.sf_mode = msr_sf;

#endif

    ctx.fpu_enabled = msr_fp;

    if ((env->flags & POWERPC_FLAG_SPE) && msr_spe)

        ctx.spe_enabled = msr_spe;

    else

        ctx.spe_enabled = 0;

    if ((env->flags & POWERPC_FLAG_VRE) && msr_vr)

        ctx.altivec_enabled = msr_vr;

    else

        ctx.altivec_enabled = 0;

    if ((env->flags & POWERPC_FLAG_SE) && msr_se)

        ctx.singlestep_enabled = CPU_SINGLE_STEP;

    else

        ctx.singlestep_enabled = 0;

    if ((env->flags & POWERPC_FLAG_BE) && msr_be)

        ctx.singlestep_enabled |= CPU_BRANCH_STEP;

    if (unlikely(env->singlestep_enabled))

        ctx.singlestep_enabled |= GDBSTUB_SINGLE_STEP;

#if defined (DO_SINGLE_STEP) && 0

    /* Single step trace mode */

    msr_se = 1;

#endif

    num_insns = 0;

    max_insns = tb->cflags & CF_COUNT_MASK;

    if (max_insns == 0)

        max_insns = CF_COUNT_MASK;



    gen_icount_start();

    /* Set env in case of segfault during code fetch */

    while (ctx.exception == POWERPC_EXCP_NONE && gen_opc_ptr < gen_opc_end) {

        if (unlikely(!TAILQ_EMPTY(&env->breakpoints))) {

            TAILQ_FOREACH(bp, &env->breakpoints, entry) {

                if (bp->pc == ctx.nip) {

                    gen_debug_exception(ctxp);

                    break;

                }

            }

        }

        if (unlikely(search_pc)) {

            j = gen_opc_ptr - gen_opc_buf;

            if (lj < j) {

                lj++;

                while (lj < j)

                    gen_opc_instr_start[lj++] = 0;

                gen_opc_pc[lj] = ctx.nip;

                gen_opc_instr_start[lj] = 1;

                gen_opc_icount[lj] = num_insns;

            }

        }

        LOG_DISAS("----------------\n");

        LOG_DISAS("nip=" ADDRX " super=%d ir=%d\n",

                  ctx.nip, ctx.mem_idx, (int)msr_ir);

        if (num_insns + 1 == max_insns && (tb->cflags & CF_LAST_IO))

            gen_io_start();

        if (unlikely(ctx.le_mode)) {

            ctx.opcode = bswap32(ldl_code(ctx.nip));

        } else {

            ctx.opcode = ldl_code(ctx.nip);

        }

        LOG_DISAS("translate opcode %08x (%02x %02x %02x) (%s)\n",

                    ctx.opcode, opc1(ctx.opcode), opc2(ctx.opcode),

                    opc3(ctx.opcode), little_endian ? "little" : "big");

        ctx.nip += 4;

        table = env->opcodes;

        num_insns++;

        handler = table[opc1(ctx.opcode)];

        if (is_indirect_opcode(handler)) {

            table = ind_table(handler);

            handler = table[opc2(ctx.opcode)];

            if (is_indirect_opcode(handler)) {

                table = ind_table(handler);

                handler = table[opc3(ctx.opcode)];

            }

        }

        /* Is opcode *REALLY* valid ? */

        if (unlikely(handler->handler == &gen_invalid)) {

            if (qemu_log_enabled()) {

                qemu_log("invalid/unsupported opcode: "

                          "%02x - %02x - %02x (%08x) " ADDRX " %d\n",

                          opc1(ctx.opcode), opc2(ctx.opcode),

                          opc3(ctx.opcode), ctx.opcode, ctx.nip - 4, (int)msr_ir);

            } else {

                printf("invalid/unsupported opcode: "

                       "%02x - %02x - %02x (%08x) " ADDRX " %d\n",

                       opc1(ctx.opcode), opc2(ctx.opcode),

                       opc3(ctx.opcode), ctx.opcode, ctx.nip - 4, (int)msr_ir);

            }

        } else {

            if (unlikely((ctx.opcode & handler->inval) != 0)) {

                if (qemu_log_enabled()) {

                    qemu_log("invalid bits: %08x for opcode: "

                              "%02x - %02x - %02x (%08x) " ADDRX "\n",

                              ctx.opcode & handler->inval, opc1(ctx.opcode),

                              opc2(ctx.opcode), opc3(ctx.opcode),

                              ctx.opcode, ctx.nip - 4);

                } else {

                    printf("invalid bits: %08x for opcode: "

                           "%02x - %02x - %02x (%08x) " ADDRX "\n",

                           ctx.opcode & handler->inval, opc1(ctx.opcode),

                           opc2(ctx.opcode), opc3(ctx.opcode),

                           ctx.opcode, ctx.nip - 4);

                }

                gen_inval_exception(ctxp, POWERPC_EXCP_INVAL_INVAL);

                break;

            }

        }

        (*(handler->handler))(&ctx);

#if defined(DO_PPC_STATISTICS)

        handler->count++;

#endif

        /* Check trace mode exceptions */

        if (unlikely(ctx.singlestep_enabled & CPU_SINGLE_STEP &&

                     (ctx.nip <= 0x100 || ctx.nip > 0xF00) &&

                     ctx.exception != POWERPC_SYSCALL &&

                     ctx.exception != POWERPC_EXCP_TRAP &&

                     ctx.exception != POWERPC_EXCP_BRANCH)) {

            gen_exception(ctxp, POWERPC_EXCP_TRACE);

        } else if (unlikely(((ctx.nip & (TARGET_PAGE_SIZE - 1)) == 0) ||

                            (env->singlestep_enabled) ||

                            num_insns >= max_insns)) {

            /* if we reach a page boundary or are single stepping, stop

             * generation

             */

            break;

        }

#if defined (DO_SINGLE_STEP)

        break;

#endif

    }

    if (tb->cflags & CF_LAST_IO)

        gen_io_end();

    if (ctx.exception == POWERPC_EXCP_NONE) {

        gen_goto_tb(&ctx, 0, ctx.nip);

    } else if (ctx.exception != POWERPC_EXCP_BRANCH) {

        if (unlikely(env->singlestep_enabled)) {

            gen_debug_exception(ctxp);

        }

        /* Generate the return instruction */

        tcg_gen_exit_tb(0);

    }

    gen_icount_end(tb, num_insns);

    *gen_opc_ptr = INDEX_op_end;

    if (unlikely(search_pc)) {

        j = gen_opc_ptr - gen_opc_buf;

        lj++;

        while (lj <= j)

            gen_opc_instr_start[lj++] = 0;

    } else {

        tb->size = ctx.nip - pc_start;

        tb->icount = num_insns;

    }

#if defined(DEBUG_DISAS)

    qemu_log_mask(CPU_LOG_TB_CPU, "---------------- excp: %04x\n", ctx.exception);

    log_cpu_state_mask(CPU_LOG_TB_CPU, env, 0);

    if (qemu_loglevel_mask(CPU_LOG_TB_IN_ASM)) {

        int flags;

        flags = env->bfd_mach;

        flags |= ctx.le_mode << 16;

        qemu_log("IN: %s\n", lookup_symbol(pc_start));

        log_target_disas(pc_start, ctx.nip - pc_start, flags);

        qemu_log("\n");

    }

#endif

}
