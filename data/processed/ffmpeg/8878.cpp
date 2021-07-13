static int decode_block(MJpegDecodeContext *s, DCTELEM *block,

                        int component, int dc_index, int ac_index, int16_t *quant_matrix)

{

    int code, i, j, level, val;

    VLC *ac_vlc;



    /* DC coef */

    val = mjpeg_decode_dc(s, dc_index);

    if (val == 0xffff) {

        dprintf("error dc\n");

        return -1;

    }

    val = val * quant_matrix[0] + s->last_dc[component];

    s->last_dc[component] = val;

    block[0] = val;

    /* AC coefs */

    ac_vlc = &s->vlcs[1][ac_index];

    i = 0;

    {OPEN_READER(re, &s->gb)

    for(;;) {

        UPDATE_CACHE(re, &s->gb);

        GET_VLC(code, re, &s->gb, s->vlcs[1][ac_index].table, 9, 2)



        /* EOB */

        if (code == 0x10)

            break;

        if (code == 0x100) {

            i += 16;

        } else {

            i += ((unsigned)code) >> 4;

            code &= 0xf;

            if(code > MIN_CACHE_BITS - 16){

                UPDATE_CACHE(re, &s->gb)

            }

            {

                int cache=GET_CACHE(re,gb);

                int sign=(~cache)>>31;

                level = (NEG_USR32(sign ^ cache,code) ^ sign) - sign;

            }



            LAST_SKIP_BITS(re, &s->gb, code)



            if (i >= 63) {

                if(i == 63){

                    j = s->scantable.permutated[63];

                    block[j] = level * quant_matrix[j];

                    break;

                }

                dprintf("error count: %d\n", i);

                return -1;

            }

            j = s->scantable.permutated[i];

            block[j] = level * quant_matrix[j];

        }

    }

    CLOSE_READER(re, &s->gb)}



    return 0;

}
