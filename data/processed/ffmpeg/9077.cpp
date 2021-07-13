static void decode_band_structure(GetBitContext *gbc, int blk, int eac3,

                                  int ecpl, int start_subband, int end_subband,

                                  const uint8_t *default_band_struct,

                                  uint8_t *band_struct, int *num_subbands,

                                  int *num_bands, int *band_sizes)

{

    int subbnd, bnd, n_subbands, n_bands, bnd_sz[22];



    n_subbands = end_subband - start_subband;



    /* decode band structure from bitstream or use default */

    if (!eac3 || get_bits1(gbc)) {

        for (subbnd = 0; subbnd < n_subbands - 1; subbnd++) {

            band_struct[subbnd] = get_bits1(gbc);

        }

    } else if (!blk) {

        memcpy(band_struct,

               &default_band_struct[start_subband+1],

               n_subbands-1);

    }

    band_struct[n_subbands-1] = 0;



    /* calculate number of bands and band sizes based on band structure.

       note that the first 4 subbands in enhanced coupling span only 6 bins

       instead of 12. */

    if (num_bands || band_sizes ) {

        n_bands = n_subbands;

        bnd_sz[0] = ecpl ? 6 : 12;

        for (bnd = 0, subbnd = 1; subbnd < n_subbands; subbnd++) {

            int subbnd_size = (ecpl && subbnd < 4) ? 6 : 12;

            if (band_struct[subbnd-1]) {

                n_bands--;

                bnd_sz[bnd] += subbnd_size;

            } else {

                bnd_sz[++bnd] = subbnd_size;

            }

        }

    }



    /* set optional output params */

    if (num_subbands)

        *num_subbands = n_subbands;

    if (num_bands)

        *num_bands = n_bands;

    if (band_sizes)

        memcpy(band_sizes, bnd_sz, sizeof(int)*n_bands);

}
