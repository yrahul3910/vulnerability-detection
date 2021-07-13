static int dxtory_decode_v2_565(AVCodecContext *avctx, AVFrame *pic,

                                const uint8_t *src, int src_size, int is_565)

{

    GetByteContext gb;

    GetBitContext  gb2;

    int nslices, slice, slice_height;

    uint32_t off, slice_size;

    uint8_t *dst;

    int ret;



    bytestream2_init(&gb, src, src_size);

    nslices = bytestream2_get_le16(&gb);

    off = FFALIGN(nslices * 4 + 2, 16);

    if (src_size < off) {

        av_log(avctx, AV_LOG_ERROR, "no slice data\n");

        return AVERROR_INVALIDDATA;

    }



    if (!nslices || avctx->height % nslices) {

        avpriv_request_sample(avctx, "%d slices for %dx%d", nslices,

                              avctx->width, avctx->height);

        return AVERROR_PATCHWELCOME;

    }



    slice_height = avctx->height / nslices;

    avctx->pix_fmt = AV_PIX_FMT_RGB24;

    if ((ret = ff_get_buffer(avctx, pic, 0)) < 0)

        return ret;



    dst = pic->data[0];

    for (slice = 0; slice < nslices; slice++) {

        slice_size = bytestream2_get_le32(&gb);



        ret = check_slice_size(avctx, src, src_size, slice_size, off);

        if (ret < 0)

            return ret;



        init_get_bits(&gb2, src + off + 16, (slice_size - 16) * 8);

        dx2_decode_slice_565(&gb2, avctx->width, slice_height, dst,

                             pic->linesize[0], is_565);



        dst += pic->linesize[0] * slice_height;

        off += slice_size;

    }



    return 0;

}
