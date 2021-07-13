static void qtrle_decode_8bpp(QtrleContext *s)

{

    int stream_ptr;

    int header;

    int start_line;

    int lines_to_change;

    signed char rle_code;

    int row_ptr, pixel_ptr;

    int row_inc = s->frame.linesize[0];

    unsigned char pi1, pi2, pi3, pi4;  /* 4 palette indices */

    unsigned char *rgb = s->frame.data[0];

    int pixel_limit = s->frame.linesize[0] * s->avctx->height;



    /* check if this frame is even supposed to change */

    if (s->size < 8)

        return;



    /* start after the chunk size */

    stream_ptr = 4;



    /* fetch the header */

    CHECK_STREAM_PTR(2);

    header = BE_16(&s->buf[stream_ptr]);

    stream_ptr += 2;



    /* if a header is present, fetch additional decoding parameters */

    if (header & 0x0008) {

        CHECK_STREAM_PTR(8);

        start_line = BE_16(&s->buf[stream_ptr]);

        stream_ptr += 4;

        lines_to_change = BE_16(&s->buf[stream_ptr]);

        stream_ptr += 4;

    } else {

        start_line = 0;

        lines_to_change = s->avctx->height;

    }



    row_ptr = row_inc * start_line;

    while (lines_to_change--) {

        CHECK_STREAM_PTR(2);

        pixel_ptr = row_ptr + (4 * (s->buf[stream_ptr++] - 1));



        while ((rle_code = (signed char)s->buf[stream_ptr++]) != -1) {

            if (rle_code == 0) {

                /* there's another skip code in the stream */

                CHECK_STREAM_PTR(1);

                pixel_ptr += (4 * (s->buf[stream_ptr++] - 1));

            } else if (rle_code < 0) {

                /* decode the run length code */

                rle_code = -rle_code;

                /* get the next 4 bytes from the stream, treat them as palette

                 * indices, and output them rle_code times */

                CHECK_STREAM_PTR(4);

                pi1 = s->buf[stream_ptr++];

                pi2 = s->buf[stream_ptr++];

                pi3 = s->buf[stream_ptr++];

                pi4 = s->buf[stream_ptr++];



                CHECK_PIXEL_PTR(rle_code * 4);



                while (rle_code--) {

                    rgb[pixel_ptr++] = pi1;

                    rgb[pixel_ptr++] = pi2;

                    rgb[pixel_ptr++] = pi3;

                    rgb[pixel_ptr++] = pi4;

                }

            } else {

                /* copy the same pixel directly to output 4 times */

                rle_code *= 4;

                CHECK_STREAM_PTR(rle_code);

                CHECK_PIXEL_PTR(rle_code);



                while (rle_code--) {

                    rgb[pixel_ptr++] = s->buf[stream_ptr++];

                }

            }

        }

        row_ptr += row_inc;

    }

}
