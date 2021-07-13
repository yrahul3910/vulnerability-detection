static int ir2_decode_frame(AVCodecContext *avctx,

                        void *data, int *got_frame,

                        AVPacket *avpkt)

{

    const uint8_t *buf = avpkt->data;

    int buf_size = avpkt->size;

    Ir2Context * const s = avctx->priv_data;

    AVFrame *picture = data;

    AVFrame * const p = &s->picture;

    int start, ret;



    if(p->data[0])

        avctx->release_buffer(avctx, p);



    p->reference = 1;

    p->buffer_hints = FF_BUFFER_HINTS_VALID | FF_BUFFER_HINTS_PRESERVE | FF_BUFFER_HINTS_REUSABLE;

    if ((ret = avctx->reget_buffer(avctx, p)) < 0) {

        av_log(s->avctx, AV_LOG_ERROR, "reget_buffer() failed\n");

        return ret;

    }



    start = 48; /* hardcoded for now */



    if (start >= buf_size) {

        av_log(s->avctx, AV_LOG_ERROR, "input buffer size too small (%d)\n", buf_size);

        return AVERROR_INVALIDDATA;

    }



    s->decode_delta = buf[18];



    /* decide whether frame uses deltas or not */

#ifndef BITSTREAM_READER_LE

    for (i = 0; i < buf_size; i++)

        buf[i] = ff_reverse[buf[i]];

#endif



    init_get_bits(&s->gb, buf + start, (buf_size - start) * 8);



    if (s->decode_delta) { /* intraframe */

        ir2_decode_plane(s, avctx->width, avctx->height,

                         s->picture.data[0], s->picture.linesize[0], ir2_luma_table);

        /* swapped U and V */

        ir2_decode_plane(s, avctx->width >> 2, avctx->height >> 2,

                         s->picture.data[2], s->picture.linesize[2], ir2_luma_table);

        ir2_decode_plane(s, avctx->width >> 2, avctx->height >> 2,

                         s->picture.data[1], s->picture.linesize[1], ir2_luma_table);

    } else { /* interframe */

        ir2_decode_plane_inter(s, avctx->width, avctx->height,

                         s->picture.data[0], s->picture.linesize[0], ir2_luma_table);

        /* swapped U and V */

        ir2_decode_plane_inter(s, avctx->width >> 2, avctx->height >> 2,

                         s->picture.data[2], s->picture.linesize[2], ir2_luma_table);

        ir2_decode_plane_inter(s, avctx->width >> 2, avctx->height >> 2,

                         s->picture.data[1], s->picture.linesize[1], ir2_luma_table);

    }



    *picture   = s->picture;

    *got_frame = 1;



    return buf_size;

}
