static void tgen_andi(TCGContext *s, TCGType type, TCGReg dest, uint64_t val)

{

    static const S390Opcode ni_insns[4] = {

        RI_NILL, RI_NILH, RI_NIHL, RI_NIHH

    };

    static const S390Opcode nif_insns[2] = {

        RIL_NILF, RIL_NIHF

    };

    uint64_t valid = (type == TCG_TYPE_I32 ? 0xffffffffull : -1ull);

    int i;



    /* Look for the zero-extensions.  */

    if ((val & valid) == 0xffffffff) {

        tgen_ext32u(s, dest, dest);

        return;

    }

    if (facilities & FACILITY_EXT_IMM) {

        if ((val & valid) == 0xff) {

            tgen_ext8u(s, TCG_TYPE_I64, dest, dest);

            return;

        }

        if ((val & valid) == 0xffff) {

            tgen_ext16u(s, TCG_TYPE_I64, dest, dest);

            return;

        }

    }



    /* Try all 32-bit insns that can perform it in one go.  */

    for (i = 0; i < 4; i++) {

        tcg_target_ulong mask = ~(0xffffull << i*16);

        if (((val | ~valid) & mask) == mask) {

            tcg_out_insn_RI(s, ni_insns[i], dest, val >> i*16);

            return;

        }

    }



    /* Try all 48-bit insns that can perform it in one go.  */

    if (facilities & FACILITY_EXT_IMM) {

        for (i = 0; i < 2; i++) {

            tcg_target_ulong mask = ~(0xffffffffull << i*32);

            if (((val | ~valid) & mask) == mask) {

                tcg_out_insn_RIL(s, nif_insns[i], dest, val >> i*32);

                return;

            }

        }

    }

    if ((facilities & FACILITY_GEN_INST_EXT) && risbg_mask(val)) {

        tgen_andi_risbg(s, dest, dest, val);

        return;

    }



    /* Fall back to loading the constant.  */

    tcg_out_movi(s, type, TCG_TMP0, val);

    if (type == TCG_TYPE_I32) {

        tcg_out_insn(s, RR, NR, dest, TCG_TMP0);

    } else {

        tcg_out_insn(s, RRE, NGR, dest, TCG_TMP0);

    }

}
