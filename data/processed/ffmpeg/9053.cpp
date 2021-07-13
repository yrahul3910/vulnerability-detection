static int txd_decode_frame(AVCodecContext *avctx, void *data, int *got_frame,

                            AVPacket *avpkt) {

    GetByteContext gb;

    AVFrame * const p = data;

    unsigned int version, w, h, d3d_format, depth, stride, flags;

    unsigned int y, v;

    uint8_t *ptr;

    uint32_t *pal;

    int ret;



    bytestream2_init(&gb, avpkt->data, avpkt->size);

    version         = bytestream2_get_le32(&gb);

    bytestream2_skip(&gb, 72);

    d3d_format      = bytestream2_get_le32(&gb);

    w               = bytestream2_get_le16(&gb);

    h               = bytestream2_get_le16(&gb);

    depth           = bytestream2_get_byte(&gb);

    bytestream2_skip(&gb, 2);

    flags           = bytestream2_get_byte(&gb);



    if (version < 8 || version > 9) {

        av_log(avctx, AV_LOG_ERROR, "texture data version %i is unsupported\n",

                                                                    version);

        return AVERROR_PATCHWELCOME;

    }



    if (depth == 8) {

        avctx->pix_fmt = AV_PIX_FMT_PAL8;

    } else if (depth == 16 || depth == 32) {

        avctx->pix_fmt = AV_PIX_FMT_RGB32;

    } else {

        av_log(avctx, AV_LOG_ERROR, "depth of %i is unsupported\n", depth);

        return AVERROR_PATCHWELCOME;

    }



    if ((ret = ff_set_dimensions(avctx, w, h)) < 0)

        return ret;



    if ((ret = ff_get_buffer(avctx, p, 0)) < 0)

        return ret;



    p->pict_type = AV_PICTURE_TYPE_I;



    ptr    = p->data[0];

    stride = p->linesize[0];



    if (depth == 8) {

        pal = (uint32_t *) p->data[1];

        for (y = 0; y < 256; y++) {

            v = bytestream2_get_be32(&gb);

            pal[y] = (v >> 8) + (v << 24);

        }

        if (bytestream2_get_bytes_left(&gb) < w * h)

            return AVERROR_INVALIDDATA;

        bytestream2_skip(&gb, 4);

        for (y=0; y<h; y++) {

            bytestream2_get_buffer(&gb, ptr, w);

            ptr += stride;

        }

    } else if (depth == 16) {

        bytestream2_skip(&gb, 4);

        switch (d3d_format) {

        case 0:

            if (!(flags & 1))

                goto unsupported;

        case FF_S3TC_DXT1:

            if (bytestream2_get_bytes_left(&gb) < (w/4) * (h/4) * 8)

                return AVERROR_INVALIDDATA;

            ff_decode_dxt1(&gb, ptr, w, h, stride);

            break;

        case FF_S3TC_DXT3:

            if (bytestream2_get_bytes_left(&gb) < (w/4) * (h/4) * 16)

                return AVERROR_INVALIDDATA;

            ff_decode_dxt3(&gb, ptr, w, h, stride);

            break;

        default:

            goto unsupported;

        }

    } else if (depth == 32) {

        switch (d3d_format) {

        case 0x15:

        case 0x16:

            if (bytestream2_get_bytes_left(&gb) < h * w * 4)

                return AVERROR_INVALIDDATA;

            for (y=0; y<h; y++) {

                bytestream2_get_buffer(&gb, ptr, w * 4);

                ptr += stride;

            }

            break;

        default:

            goto unsupported;

        }

    }



    *got_frame = 1;



    return avpkt->size;



unsupported:

    av_log(avctx, AV_LOG_ERROR, "unsupported d3d format (%08x)\n", d3d_format);

    return AVERROR_PATCHWELCOME;

}
