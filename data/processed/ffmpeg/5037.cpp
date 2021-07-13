static int cinepak_decode_frame(AVCodecContext *avctx,

                                void *data, int *got_frame,

                                AVPacket *avpkt)

{

    const uint8_t *buf = avpkt->data;

    int ret = 0, buf_size = avpkt->size;

    CinepakContext *s = avctx->priv_data;



    s->data = buf;

    s->size = buf_size;



    if ((ret = ff_reget_buffer(avctx, s->frame)) < 0)

        return ret;



    if (s->palette_video) {

        const uint8_t *pal = av_packet_get_side_data(avpkt, AV_PKT_DATA_PALETTE, NULL);

        if (pal) {

            s->frame->palette_has_changed = 1;

            memcpy(s->pal, pal, AVPALETTE_SIZE);

        }

    }



    if ((ret = cinepak_decode(s)) < 0) {

        av_log(avctx, AV_LOG_ERROR, "cinepak_decode failed\n");

    }



    if (s->palette_video)

        memcpy (s->frame->data[1], s->pal, AVPALETTE_SIZE);



    if ((ret = av_frame_ref(data, s->frame)) < 0)

        return ret;



    *got_frame = 1;



    /* report that the buffer was completely consumed */

    return buf_size;

}
