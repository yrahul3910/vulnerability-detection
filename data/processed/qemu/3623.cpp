static inline void gen_evfsneg(DisasContext *ctx)

{

    if (unlikely(!ctx->spe_enabled)) {

        gen_exception(ctx, POWERPC_EXCP_APU);

        return;

    }

#if defined(TARGET_PPC64)

    tcg_gen_xori_tl(cpu_gpr[rD(ctx->opcode)], cpu_gpr[rA(ctx->opcode)], 0x8000000080000000LL);

#else

    tcg_gen_xori_tl(cpu_gpr[rD(ctx->opcode)], cpu_gpr[rA(ctx->opcode)], 0x80000000);

    tcg_gen_xori_tl(cpu_gprh[rD(ctx->opcode)], cpu_gprh[rA(ctx->opcode)], 0x80000000);

#endif

}
