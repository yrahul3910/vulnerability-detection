static int decode_dvd_subtitles(AVSubtitle *sub_header,

                                const uint8_t *buf, int buf_size)

{

    int cmd_pos, pos, cmd, x1, y1, x2, y2, offset1, offset2, next_cmd_pos;

    int big_offsets, offset_size, is_8bit = 0;

    const uint8_t *yuv_palette = 0;

    uint8_t colormap[4], alpha[256];

    int date;

    int i;

    int is_menu = 0;



    if (buf_size < 10)

        return -1;

    sub_header->rects = NULL;

    sub_header->num_rects = 0;

    sub_header->format = 0;

    sub_header->start_display_time = 0;

    sub_header->end_display_time = 0;



    if (AV_RB16(buf) == 0) {   /* HD subpicture with 4-byte offsets */

        big_offsets = 1;

        offset_size = 4;

        cmd_pos = 6;

    } else {

        big_offsets = 0;

        offset_size = 2;

        cmd_pos = 2;

    }



    cmd_pos = READ_OFFSET(buf + cmd_pos);



    while ((cmd_pos + 2 + offset_size) < buf_size) {

        date = AV_RB16(buf + cmd_pos);

        next_cmd_pos = READ_OFFSET(buf + cmd_pos + 2);

        dprintf(NULL, "cmd_pos=0x%04x next=0x%04x date=%d\n",

                cmd_pos, next_cmd_pos, date);

        pos = cmd_pos + 2 + offset_size;

        offset1 = -1;

        offset2 = -1;

        x1 = y1 = x2 = y2 = 0;

        while (pos < buf_size) {

            cmd = buf[pos++];

            dprintf(NULL, "cmd=%02x\n", cmd);

            switch(cmd) {

            case 0x00:

                /* menu subpicture */

                is_menu = 1;

                break;

            case 0x01:

                /* set start date */

                sub_header->start_display_time = (date << 10) / 90;

                break;

            case 0x02:

                /* set end date */

                sub_header->end_display_time = (date << 10) / 90;

                break;

            case 0x03:

                /* set colormap */

                if ((buf_size - pos) < 2)

                    goto fail;

                colormap[3] = buf[pos] >> 4;

                colormap[2] = buf[pos] & 0x0f;

                colormap[1] = buf[pos + 1] >> 4;

                colormap[0] = buf[pos + 1] & 0x0f;

                pos += 2;

                break;

            case 0x04:

                /* set alpha */

                if ((buf_size - pos) < 2)

                    goto fail;

                alpha[3] = buf[pos] >> 4;

                alpha[2] = buf[pos] & 0x0f;

                alpha[1] = buf[pos + 1] >> 4;

                alpha[0] = buf[pos + 1] & 0x0f;

                pos += 2;

            dprintf(NULL, "alpha=%x%x%x%x\n", alpha[0],alpha[1],alpha[2],alpha[3]);

                break;

            case 0x05:

            case 0x85:

                if ((buf_size - pos) < 6)

                    goto fail;

                x1 = (buf[pos] << 4) | (buf[pos + 1] >> 4);

                x2 = ((buf[pos + 1] & 0x0f) << 8) | buf[pos + 2];

                y1 = (buf[pos + 3] << 4) | (buf[pos + 4] >> 4);

                y2 = ((buf[pos + 4] & 0x0f) << 8) | buf[pos + 5];

                if (cmd & 0x80)

                    is_8bit = 1;

                dprintf(NULL, "x1=%d x2=%d y1=%d y2=%d\n", x1, x2, y1, y2);

                pos += 6;

                break;

            case 0x06:

                if ((buf_size - pos) < 4)

                    goto fail;

                offset1 = AV_RB16(buf + pos);

                offset2 = AV_RB16(buf + pos + 2);

                dprintf(NULL, "offset1=0x%04x offset2=0x%04x\n", offset1, offset2);

                pos += 4;

                break;

            case 0x86:

                if ((buf_size - pos) < 8)

                    goto fail;

                offset1 = AV_RB32(buf + pos);

                offset2 = AV_RB32(buf + pos + 4);

                dprintf(NULL, "offset1=0x%04x offset2=0x%04x\n", offset1, offset2);

                pos += 8;

                break;



            case 0x83:

                /* HD set palette */

                if ((buf_size - pos) < 768)

                    goto fail;

                yuv_palette = buf + pos;

                pos += 768;

                break;

            case 0x84:

                /* HD set contrast (alpha) */

                if ((buf_size - pos) < 256)

                    goto fail;

                for (i = 0; i < 256; i++)

                    alpha[i] = 0xFF - buf[pos+i];

                pos += 256;

                break;



            case 0xff:

                goto the_end;

            default:

                dprintf(NULL, "unrecognised subpicture command 0x%x\n", cmd);

                goto the_end;

            }

        }

    the_end:

        if (offset1 >= 0) {

            int w, h;

            uint8_t *bitmap;



            /* decode the bitmap */

            w = x2 - x1 + 1;

            if (w < 0)

                w = 0;

            h = y2 - y1;

            if (h < 0)

                h = 0;

            if (w > 0 && h > 0) {

                if (sub_header->rects != NULL) {

                    for (i = 0; i < sub_header->num_rects; i++) {

                        av_freep(&sub_header->rects[i]->pict.data[0]);

                        av_freep(&sub_header->rects[i]->pict.data[1]);

                        av_freep(&sub_header->rects[i]);

                    }

                    av_freep(&sub_header->rects);

                    sub_header->num_rects = 0;

                }



                bitmap = av_malloc(w * h);

                sub_header->rects = av_mallocz(sizeof(*sub_header->rects));

                sub_header->rects[0] = av_mallocz(sizeof(AVSubtitleRect));

                sub_header->num_rects = 1;

                sub_header->rects[0]->pict.data[0] = bitmap;

                decode_rle(bitmap, w * 2, w, (h + 1) / 2,

                           buf, offset1, buf_size, is_8bit);

                decode_rle(bitmap + w, w * 2, w, h / 2,

                           buf, offset2, buf_size, is_8bit);

                if (is_8bit) {

                    if (yuv_palette == 0)

                        goto fail;

                    sub_header->rects[0]->pict.data[1] = av_malloc(256 * 4);

                    sub_header->rects[0]->nb_colors = 256;

                    yuv_a_to_rgba(yuv_palette, alpha, (uint32_t*)sub_header->rects[0]->pict.data[1], 256);

                } else {

                    sub_header->rects[0]->pict.data[1] = av_malloc(4 * 4);

                    sub_header->rects[0]->nb_colors = 4;

                    guess_palette((uint32_t*)sub_header->rects[0]->pict.data[1],

                                  colormap, alpha, 0xffff00);

                }

                sub_header->rects[0]->x = x1;

                sub_header->rects[0]->y = y1;

                sub_header->rects[0]->w = w;

                sub_header->rects[0]->h = h;

                sub_header->rects[0]->type = SUBTITLE_BITMAP;

                sub_header->rects[0]->pict.linesize[0] = w;

            }

        }

        if (next_cmd_pos == cmd_pos)

            break;

        cmd_pos = next_cmd_pos;

    }

    if (sub_header->num_rects > 0)

        return is_menu;

 fail:

    if (sub_header->rects != NULL) {

        for (i = 0; i < sub_header->num_rects; i++) {

            av_freep(&sub_header->rects[i]->pict.data[0]);

            av_freep(&sub_header->rects[i]->pict.data[1]);

            av_freep(&sub_header->rects[i]);

        }

        av_freep(&sub_header->rects);

        sub_header->num_rects = 0;

    }

    return -1;

}
