static int get_mmu_address(CPUState * env, target_ulong * physical,

			   int *prot, target_ulong address,

			   int rw, int access_type)

{

    int use_asid, n;

    tlb_t *matching = NULL;



    use_asid = (env->mmucr & MMUCR_SV) == 0 || (env->sr & SR_MD) == 0;



    if (rw == 2) {

	n = find_itlb_entry(env, address, use_asid, 1);

	if (n >= 0) {

	    matching = &env->itlb[n];

	    if ((env->sr & SR_MD) & !(matching->pr & 2))

		n = MMU_ITLB_VIOLATION;

	    else

		*prot = PAGE_READ;

	}

    } else {

	n = find_utlb_entry(env, address, use_asid);

	if (n >= 0) {

	    matching = &env->utlb[n];

	    switch ((matching->pr << 1) | ((env->sr & SR_MD) ? 1 : 0)) {

	    case 0:		/* 000 */

	    case 2:		/* 010 */

		n = (rw == 1) ? MMU_DTLB_VIOLATION_WRITE :

		    MMU_DTLB_VIOLATION_READ;

		break;

	    case 1:		/* 001 */

	    case 4:		/* 100 */

	    case 5:		/* 101 */

		if (rw == 1)

		    n = MMU_DTLB_VIOLATION_WRITE;

		else

		    *prot = PAGE_READ;

		break;

	    case 3:		/* 011 */

	    case 6:		/* 110 */

	    case 7:		/* 111 */

		*prot = (rw == 1)? PAGE_WRITE : PAGE_READ;

		break;

	    }

	} else if (n == MMU_DTLB_MISS) {

	    n = (rw == 1) ? MMU_DTLB_MISS_WRITE :

		MMU_DTLB_MISS_READ;

	}

    }

    if (n >= 0) {

	*physical = ((matching->ppn << 10) & ~(matching->size - 1)) |

	    (address & (matching->size - 1));

	if ((rw == 1) & !matching->d)

	    n = MMU_DTLB_INITIAL_WRITE;

	else

	    n = MMU_OK;

    }

    return n;

}
