static inline void asv2_encode_block(ASV1Context *a, int16_t block[64])

{

    int i;

    int count = 0;



    for (count = 63; count > 3; count--) {

        const int index = ff_asv_scantab[count];

        if ((block[index] * a->q_intra_matrix[index] + (1 << 15)) >> 16)

            break;

    }



    count >>= 2;



    asv2_put_bits(&a->pb, 4, count);

    asv2_put_bits(&a->pb, 8, (block[0] + 32) >> 6);

    block[0] = 0;



    for (i = 0; i <= count; i++) {

        const int index = ff_asv_scantab[4 * i];

        int ccp         = 0;



        if ((block[index + 0] = (block[index + 0] *

                                 a->q_intra_matrix[index + 0] + (1 << 15)) >> 16))

            ccp |= 8;

        if ((block[index + 8] = (block[index + 8] *

                                 a->q_intra_matrix[index + 8] + (1 << 15)) >> 16))

            ccp |= 4;

        if ((block[index + 1] = (block[index + 1] *

                                 a->q_intra_matrix[index + 1] + (1 << 15)) >> 16))

            ccp |= 2;

        if ((block[index + 9] = (block[index + 9] *

                                 a->q_intra_matrix[index + 9] + (1 << 15)) >> 16))

            ccp |= 1;



        av_assert2(i || ccp < 8);

        if (i)

            put_bits(&a->pb, ff_asv_ac_ccp_tab[ccp][1], ff_asv_ac_ccp_tab[ccp][0]);

        else

            put_bits(&a->pb, ff_asv_dc_ccp_tab[ccp][1], ff_asv_dc_ccp_tab[ccp][0]);



        if (ccp) {

            if (ccp & 8)

                asv2_put_level(&a->pb, block[index + 0]);

            if (ccp & 4)

                asv2_put_level(&a->pb, block[index + 8]);

            if (ccp & 2)

                asv2_put_level(&a->pb, block[index + 1]);

            if (ccp & 1)

                asv2_put_level(&a->pb, block[index + 9]);

        }

    }

}
