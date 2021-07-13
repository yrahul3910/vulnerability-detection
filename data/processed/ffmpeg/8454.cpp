static void decode_gray_bitstream(HYuvContext *s, int count)

{

    int i;

    OPEN_READER(re, &s->gb);

    count /= 2;



    if (count >= (get_bits_left(&s->gb)) / (32 * 2)) {

        for (i = 0; i < count && get_bits_left(&s->gb) > 0; i++) {

            READ_2PIX(s->temp[0][2 * i], s->temp[0][2 * i + 1], 0);

        }

    } else {

        for (i = 0; i < count; i++) {

            READ_2PIX(s->temp[0][2 * i], s->temp[0][2 * i + 1], 0);

        }

    }

    CLOSE_READER(re, &s->gb);

}
