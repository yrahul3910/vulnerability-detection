static void gif_put_bits_rev(PutBitContext *s, int n, unsigned int value)

{

    unsigned int bit_buf;

    int bit_cnt;



    //    printf("put_bits=%d %x\n", n, value);

    assert(n == 32 || value < (1U << n));



    bit_buf = s->bit_buf;

    bit_cnt = 32 - s->bit_left; /* XXX:lazyness... was = s->bit_cnt; */



    //    printf("n=%d value=%x cnt=%d buf=%x\n", n, value, bit_cnt, bit_buf);

    /* XXX: optimize */

    if (n < (32-bit_cnt)) {

        bit_buf |= value << (bit_cnt);

        bit_cnt+=n;

    } else {

        bit_buf |= value << (bit_cnt);



        *s->buf_ptr = bit_buf & 0xff;

        s->buf_ptr[1] = (bit_buf >> 8) & 0xff;

        s->buf_ptr[2] = (bit_buf >> 16) & 0xff;

        s->buf_ptr[3] = (bit_buf >> 24) & 0xff;



        //printf("bitbuf = %08x\n", bit_buf);

        s->buf_ptr+=4;

        if (s->buf_ptr >= s->buf_end)

            puts("bit buffer overflow !!"); // should never happen ! who got rid of the callback ???

//            flush_buffer_rev(s);

        bit_cnt=bit_cnt + n - 32;

        if (bit_cnt == 0) {

            bit_buf = 0;

        } else {

            bit_buf = value >> (n - bit_cnt);

        }

    }



    s->bit_buf = bit_buf;

    s->bit_left = 32 - bit_cnt;

}
