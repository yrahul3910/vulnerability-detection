void do_subfzeo_64 (void)

{

    T1 = T0;

    T0 = ~T0 + xer_ca;

    if (likely(!(((uint64_t)~T1 ^ UINT64_MAX) &

                 ((uint64_t)(~T1) ^ (uint64_t)T0) & (1ULL << 63)))) {

        xer_ov = 0;

    } else {

        xer_ov = 1;

        xer_so = 1;

    }

    if (likely((uint64_t)T0 >= (uint64_t)~T1)) {

        xer_ca = 0;

    } else {

        xer_ca = 1;

    }

}
