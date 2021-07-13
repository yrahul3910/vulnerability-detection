static ExitStatus trans_fop_dedd(DisasContext *ctx, uint32_t insn,

                                 const DisasInsn *di)

{

    unsigned rt = extract32(insn, 0, 5);

    unsigned rb = extract32(insn, 16, 5);

    unsigned ra = extract32(insn, 21, 5);

    return do_fop_dedd(ctx, rt, ra, rb, di->f_dedd);

}
