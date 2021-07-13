static inline void gen_op_eval_fbu(TCGv dst, TCGv src,

                                    unsigned int fcc_offset)

{

    gen_mov_reg_FCC0(dst, src, fcc_offset);

    gen_mov_reg_FCC1(cpu_tmp0, src, fcc_offset);

    tcg_gen_and_tl(dst, dst, cpu_tmp0);

}
