static int decode_frame_png(AVCodecContext *avctx,

                        void *data, int *got_frame,

                        AVPacket *avpkt)

{

    PNGDecContext *const s = avctx->priv_data;

    const uint8_t *buf     = avpkt->data;

    int buf_size           = avpkt->size;

    AVFrame *p;

    int64_t sig;

    int ret;



    ff_thread_release_buffer(avctx, &s->last_picture);

    FFSWAP(ThreadFrame, s->picture, s->last_picture);

    p = s->picture.f;



    bytestream2_init(&s->gb, buf, buf_size);



    /* check signature */

    sig = bytestream2_get_be64(&s->gb);

    if (sig != PNGSIG &&

        sig != MNGSIG) {

        av_log(avctx, AV_LOG_ERROR, "Invalid PNG signature 0x%08"PRIX64".\n", sig);

        return AVERROR_INVALIDDATA;

    }



    s->y = s->has_trns = 0;

    s->hdr_state = 0;

    s->pic_state = 0;



    /* init the zlib */

    s->zstream.zalloc = ff_png_zalloc;

    s->zstream.zfree  = ff_png_zfree;

    s->zstream.opaque = NULL;

    ret = inflateInit(&s->zstream);

    if (ret != Z_OK) {

        av_log(avctx, AV_LOG_ERROR, "inflateInit returned error %d\n", ret);

        return AVERROR_EXTERNAL;

    }



    if ((ret = decode_frame_common(avctx, s, p, avpkt)) < 0)

        goto the_end;



    if (avctx->skip_frame == AVDISCARD_ALL) {

        *got_frame = 0;

        ret = bytestream2_tell(&s->gb);

        goto the_end;

    }



    if ((ret = av_frame_ref(data, s->picture.f)) < 0)

        return ret;



    *got_frame = 1;



    ret = bytestream2_tell(&s->gb);

the_end:

    inflateEnd(&s->zstream);

    s->crow_buf = NULL;

    return ret;

}
