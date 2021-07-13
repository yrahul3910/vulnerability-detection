static int aasc_decode_frame(AVCodecContext *avctx,

                              void *data, int *got_frame,

                              AVPacket *avpkt)

{

    const uint8_t *buf = avpkt->data;

    int buf_size       = avpkt->size;

    AascContext *s     = avctx->priv_data;

    int compr, i, stride, ret;



    s->frame.reference = 1;

    s->frame.buffer_hints = FF_BUFFER_HINTS_VALID | FF_BUFFER_HINTS_PRESERVE | FF_BUFFER_HINTS_REUSABLE;

    if ((ret = avctx->reget_buffer(avctx, &s->frame)) < 0) {

        av_log(avctx, AV_LOG_ERROR, "reget_buffer() failed\n");

        return ret;

    }



    compr     = AV_RL32(buf);

    buf      += 4;

    buf_size -= 4;

    switch (compr) {

    case 0:

        stride = (avctx->width * 3 + 3) & ~3;

        for (i = avctx->height - 1; i >= 0; i--) {

            memcpy(s->frame.data[0] + i * s->frame.linesize[0], buf, avctx->width * 3);

            buf += stride;

        }

        break;

    case 1:

        bytestream2_init(&s->gb, buf - 4, buf_size + 4);

        ff_msrle_decode(avctx, (AVPicture*)&s->frame, 8, &s->gb);

        break;

    default:

        av_log(avctx, AV_LOG_ERROR, "Unknown compression type %d\n", compr);

        return AVERROR_INVALIDDATA;

    }



    *got_frame = 1;

    *(AVFrame*)data = s->frame;



    /* report that the buffer was completely consumed */

    return buf_size;

}
