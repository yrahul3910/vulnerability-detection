static int dvbsub_parse_clut_segment(AVCodecContext *avctx,

                                     const uint8_t *buf, int buf_size)

{

    DVBSubContext *ctx = avctx->priv_data;



    const uint8_t *buf_end = buf + buf_size;

    int i, clut_id;

    int version;

    DVBSubCLUT *clut;

    int entry_id, depth , full_range;

    int y, cr, cb, alpha;

    int r, g, b, r_add, g_add, b_add;



    ff_dlog(avctx, "DVB clut packet:\n");



    for (i=0; i < buf_size; i++) {

        ff_dlog(avctx, "%02x ", buf[i]);

        if (i % 16 == 15)

            ff_dlog(avctx, "\n");

    }



    if (i % 16)

        ff_dlog(avctx, "\n");



    clut_id = *buf++;

    version = ((*buf)>>4)&15;

    buf += 1;



    clut = get_clut(ctx, clut_id);



    if (!clut) {

        clut = av_malloc(sizeof(DVBSubCLUT));

        if (!clut)

            return AVERROR(ENOMEM);



        memcpy(clut, &default_clut, sizeof(DVBSubCLUT));



        clut->id = clut_id;

        clut->version = -1;



        clut->next = ctx->clut_list;

        ctx->clut_list = clut;

    }



    if (clut->version != version) {



    clut->version = version;



    while (buf + 4 < buf_end) {

        entry_id = *buf++;



        depth = (*buf) & 0xe0;



        if (depth == 0) {

            av_log(avctx, AV_LOG_ERROR, "Invalid clut depth 0x%x!\n", *buf);

            return 0;

        }



        full_range = (*buf++) & 1;



        if (full_range) {

            y = *buf++;

            cr = *buf++;

            cb = *buf++;

            alpha = *buf++;

        } else {

            y = buf[0] & 0xfc;

            cr = (((buf[0] & 3) << 2) | ((buf[1] >> 6) & 3)) << 4;

            cb = (buf[1] << 2) & 0xf0;

            alpha = (buf[1] << 6) & 0xc0;



            buf += 2;

        }



        if (y == 0)

            alpha = 0xff;



        YUV_TO_RGB1_CCIR(cb, cr);

        YUV_TO_RGB2_CCIR(r, g, b, y);



        ff_dlog(avctx, "clut %d := (%d,%d,%d,%d)\n", entry_id, r, g, b, alpha);

        if (!!(depth & 0x80) + !!(depth & 0x40) + !!(depth & 0x20) > 1) {

            ff_dlog(avctx, "More than one bit level marked: %x\n", depth);

            if (avctx->strict_std_compliance > FF_COMPLIANCE_NORMAL)

                return AVERROR_INVALIDDATA;

        }



        if (depth & 0x80)

            clut->clut4[entry_id] = RGBA(r,g,b,255 - alpha);

        else if (depth & 0x40)

            clut->clut16[entry_id] = RGBA(r,g,b,255 - alpha);

        else if (depth & 0x20)

            clut->clut256[entry_id] = RGBA(r,g,b,255 - alpha);

    }

    }



    return 0;

}
