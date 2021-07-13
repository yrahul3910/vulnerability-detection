yuv2planeX_16_c_template(const int16_t *filter, int filterSize,

                         const int32_t **src, uint16_t *dest, int dstW,

                         int big_endian, int output_bits)

{

    int i;

    int shift = 15;

    av_assert0(output_bits == 16);



    for (i = 0; i < dstW; i++) {

        int val = 1 << (shift - 1);

        int j;



        /* range of val is [0,0x7FFFFFFF], so 31 bits, but with lanczos/spline

         * filters (or anything with negative coeffs, the range can be slightly

         * wider in both directions. To account for this overflow, we subtract

         * a constant so it always fits in the signed range (assuming a

         * reasonable filterSize), and re-add that at the end. */

        val -= 0x40000000;

        for (j = 0; j < filterSize; j++)

            val += src[j][i] * filter[j];



        output_pixel(&dest[i], val, 0x8000, int);

    }

}
