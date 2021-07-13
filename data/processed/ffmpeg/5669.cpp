static void decode_scaling_list(H264Context *h, uint8_t *factors, int size,

                                const uint8_t *jvt_list,

                                const uint8_t *fallback_list)

{

    int i, last = 8, next = 8;

    const uint8_t *scan = size == 16 ? ff_zigzag_scan : ff_zigzag_direct;

    if (!get_bits1(&h->gb)) /* matrix not written, we use the predicted one */

        memcpy(factors, fallback_list, size * sizeof(uint8_t));

    else

        for (i = 0; i < size; i++) {

            if (next)

                next = (last + get_se_golomb(&h->gb)) & 0xff;

            if (!i && !next) { /* matrix not written, we use the preset one */

                memcpy(factors, jvt_list, size * sizeof(uint8_t));

                break;

            }

            last = factors[scan[i]] = next ? next : last;

        }

}
