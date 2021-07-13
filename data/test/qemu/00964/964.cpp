static target_ulong h_enter(CPUState *env, sPAPREnvironment *spapr,

                            target_ulong opcode, target_ulong *args)

{

    target_ulong flags = args[0];

    target_ulong pte_index = args[1];

    target_ulong pteh = args[2];

    target_ulong ptel = args[3];

    target_ulong i;

    uint8_t *hpte;



    /* only handle 4k and 16M pages for now */

    if (pteh & HPTE_V_LARGE) {

#if 0 /* We don't support 64k pages yet */

        if ((ptel & 0xf000) == 0x1000) {

            /* 64k page */

        } else

#endif

        if ((ptel & 0xff000) == 0) {

            /* 16M page */

            /* lowest AVA bit must be 0 for 16M pages */

            if (pteh & 0x80) {

                return H_PARAMETER;

            }

        } else {

            return H_PARAMETER;

        }

    }



    /* FIXME: bounds check the pa? */



    /* Check WIMG */

    if ((ptel & HPTE_R_WIMG) != HPTE_R_M) {

        return H_PARAMETER;

    }

    pteh &= ~0x60ULL;



    if ((pte_index * HASH_PTE_SIZE_64) & ~env->htab_mask) {

        return H_PARAMETER;

    }

    if (likely((flags & H_EXACT) == 0)) {

        pte_index &= ~7ULL;

        hpte = env->external_htab + (pte_index * HASH_PTE_SIZE_64);

        for (i = 0; ; ++i) {

            if (i == 8) {

                return H_PTEG_FULL;

            }

            if (((ldq_p(hpte) & HPTE_V_VALID) == 0) &&

                lock_hpte(hpte, HPTE_V_HVLOCK | HPTE_V_VALID)) {

                break;

            }

            hpte += HASH_PTE_SIZE_64;

        }

    } else {

        i = 0;

        hpte = env->external_htab + (pte_index * HASH_PTE_SIZE_64);

        if (!lock_hpte(hpte, HPTE_V_HVLOCK | HPTE_V_VALID)) {

            return H_PTEG_FULL;

        }

    }

    stq_p(hpte + (HASH_PTE_SIZE_64/2), ptel);

    /* eieio();  FIXME: need some sort of barrier for smp? */

    stq_p(hpte, pteh);



    assert(!(ldq_p(hpte) & HPTE_V_HVLOCK));

    args[0] = pte_index + i;

    return H_SUCCESS;

}
