static void spr_read_tbu(DisasContext *ctx, int gprn, int sprn)

{

    if (ctx->tb->cflags & CF_USE_ICOUNT) {

        gen_io_start();

    }

    gen_helper_load_tbu(cpu_gpr[gprn], cpu_env);

    if (ctx->tb->cflags & CF_USE_ICOUNT) {

        gen_io_end();

        gen_stop_exception(ctx);

    }

}
