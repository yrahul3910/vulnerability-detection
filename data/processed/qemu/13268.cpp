static inline void gen_jcc1_noeob(DisasContext *s, int b, int l1)

{

    CCPrepare cc = gen_prepare_cc(s, b, cpu_T[0]);



    if (cc.mask != -1) {

        tcg_gen_andi_tl(cpu_T[0], cc.reg, cc.mask);

        cc.reg = cpu_T[0];

    }

    if (cc.use_reg2) {

        tcg_gen_brcond_tl(cc.cond, cc.reg, cc.reg2, l1);

    } else {

        tcg_gen_brcondi_tl(cc.cond, cc.reg, cc.imm, l1);

    }

}
