void OPPROTO op_check_addo (void)

{

    if (likely(!(((uint32_t)T2 ^ (uint32_t)T1 ^ UINT32_MAX) &

                 ((uint32_t)T2 ^ (uint32_t)T0) & (1UL << 31)))) {

        xer_ov = 0;

    } else {

        xer_so = 1;

        xer_ov = 1;

    }

    RETURN();

}
