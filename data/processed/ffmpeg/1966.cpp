void ff_simple_idct84_add(uint8_t *dest, int line_size, DCTELEM *block)

{

    int i;



    /* IDCT8 on each line */

    for(i=0; i<4; i++) {

        idctRowCondDC_8(block + i*8);

    }



    /* IDCT4 and store */

    for(i=0;i<8;i++) {

        idct4col_add(dest + i, line_size, block + i);

    }

}
