static int decode_frame(AVCodecContext *avctx,
                        void *data, int *data_size,
                        AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    const uint8_t *buf_end = avpkt->data + avpkt->size;
    int buf_size = avpkt->size;
    QdrawContext * const a = avctx->priv_data;
    AVFrame * const p= (AVFrame*)&a->pic;
    uint8_t* outdata;
    int colors;
    int i;
    uint32_t *pal;
    int r, g, b;
    if(p->data[0])
        avctx->release_buffer(avctx, p);
    p->reference= 0;
    if(avctx->get_buffer(avctx, p) < 0){
        av_log(avctx, AV_LOG_ERROR, "get_buffer() failed\n");
        return -1;
    }
    p->pict_type= AV_PICTURE_TYPE_I;
    p->key_frame= 1;
    outdata = a->pic.data[0];
    if (buf_end - buf < 0x68 + 4)
    buf += 0x68; /* jump to palette */
    colors = AV_RB32(buf);
    buf += 4;
    if(colors < 0 || colors > 256) {
        av_log(avctx, AV_LOG_ERROR, "Error color count - %i(0x%X)\n", colors, colors);
        return -1;
    }
    if (buf_end - buf < (colors + 1) * 8)
    pal = (uint32_t*)p->data[1];
    for (i = 0; i <= colors; i++) {
        unsigned int idx;
        idx = AV_RB16(buf); /* color index */
        buf += 2;
        if (idx > 255) {
            av_log(avctx, AV_LOG_ERROR, "Palette index out of range: %u\n", idx);
            buf += 6;
            continue;
        }
        r = *buf++;
        buf++;
        g = *buf++;
        buf++;
        b = *buf++;
        buf++;
        pal[idx] = (r << 16) | (g << 8) | b;
    }
    p->palette_has_changed = 1;
    if (buf_end - buf < 18)
    buf += 18; /* skip unneeded data */
    for (i = 0; i < avctx->height; i++) {
        int size, left, code, pix;
        const uint8_t *next;
        uint8_t *out;
        int tsize = 0;
        /* decode line */
        out = outdata;
        size = AV_RB16(buf); /* size of packed line */
        buf += 2;
        left = size;
        next = buf + size;
        while (left > 0) {
            code = *buf++;
            if (code & 0x80 ) { /* run */
                pix = *buf++;
                if ((out + (257 - code)) > (outdata +  a->pic.linesize[0]))
                    break;
                memset(out, pix, 257 - code);
                out += 257 - code;
                tsize += 257 - code;
                left -= 2;
            } else { /* copy */
                if ((out + code) > (outdata +  a->pic.linesize[0]))
                    break;
                if (buf_end - buf < code + 1)
                memcpy(out, buf, code + 1);
                out += code + 1;
                buf += code + 1;
                left -= 2 + code;
                tsize += code + 1;
            }
        }
        buf = next;
        outdata += a->pic.linesize[0];
    }
    *data_size = sizeof(AVFrame);
    *(AVFrame*)data = a->pic;
    return buf_size;
}