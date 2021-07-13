static void gen_brcond(DisasContext *dc, TCGCond cond,

        TCGv_i32 t0, TCGv_i32 t1, uint32_t offset)

{

    int label = gen_new_label();



    gen_advance_ccount(dc);

    tcg_gen_brcond_i32(cond, t0, t1, label);

    gen_jumpi_check_loop_end(dc, 0);

    gen_set_label(label);

    gen_jumpi(dc, dc->pc + offset, 1);

}
