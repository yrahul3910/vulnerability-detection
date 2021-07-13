int ff_mjpeg_decode_sof(MJpegDecodeContext *s)

{

    int len, nb_components, i, width, height, pix_fmt_id;



    s->cur_scan = 0;

    s->upscale_h = s->upscale_v = 0;



    /* XXX: verify len field validity */

    len     = get_bits(&s->gb, 16);

    s->bits = get_bits(&s->gb, 8);



    if (s->pegasus_rct)

        s->bits = 9;

    if (s->bits == 9 && !s->pegasus_rct)

        s->rct  = 1;    // FIXME ugly



    if (s->bits != 8 && !s->lossless) {

        av_log(s->avctx, AV_LOG_ERROR, "only 8 bits/component accepted\n");

        return -1;

    }



    if(s->lossless && s->avctx->lowres){

        av_log(s->avctx, AV_LOG_ERROR, "lowres is not possible with lossless jpeg\n");

        return -1;

    }



    height = get_bits(&s->gb, 16);

    width  = get_bits(&s->gb, 16);



    // HACK for odd_height.mov

    if (s->interlaced && s->width == width && s->height == height + 1)

        height= s->height;



    av_log(s->avctx, AV_LOG_DEBUG, "sof0: picture: %dx%d\n", width, height);

    if (av_image_check_size(width, height, 0, s->avctx))

        return AVERROR_INVALIDDATA;



    nb_components = get_bits(&s->gb, 8);

    if (nb_components <= 0 ||

        nb_components > MAX_COMPONENTS)

        return -1;

    if (s->interlaced && (s->bottom_field == !s->interlace_polarity)) {

        if (nb_components != s->nb_components) {

            av_log(s->avctx, AV_LOG_ERROR, "nb_components changing in interlaced picture\n");

            return AVERROR_INVALIDDATA;

        }

    }

    if (s->ls && !(s->bits <= 8 || nb_components == 1)) {

        av_log_missing_feature(s->avctx,

                               "For JPEG-LS anything except <= 8 bits/component"

                               " or 16-bit gray", 0);

        return AVERROR_PATCHWELCOME;

    }

    s->nb_components = nb_components;

    s->h_max         = 1;

    s->v_max         = 1;

    memset(s->h_count, 0, sizeof(s->h_count));

    memset(s->v_count, 0, sizeof(s->v_count));

    for (i = 0; i < nb_components; i++) {

        /* component id */

        s->component_id[i] = get_bits(&s->gb, 8) - 1;

        s->h_count[i]      = get_bits(&s->gb, 4);

        s->v_count[i]      = get_bits(&s->gb, 4);

        /* compute hmax and vmax (only used in interleaved case) */

        if (s->h_count[i] > s->h_max)

            s->h_max = s->h_count[i];

        if (s->v_count[i] > s->v_max)

            s->v_max = s->v_count[i];

        if (!s->h_count[i] || !s->v_count[i]) {

            av_log(s->avctx, AV_LOG_ERROR, "h/v_count is 0\n");

            return -1;

        }

        s->quant_index[i] = get_bits(&s->gb, 8);

        if (s->quant_index[i] >= 4)

            return AVERROR_INVALIDDATA;

        av_log(s->avctx, AV_LOG_DEBUG, "component %d %d:%d id: %d quant:%d\n",

               i, s->h_count[i], s->v_count[i],

               s->component_id[i], s->quant_index[i]);

    }



    if (s->ls && (s->h_max > 1 || s->v_max > 1)) {

        av_log_missing_feature(s->avctx, "Subsampling in JPEG-LS", 0);

        return AVERROR_PATCHWELCOME;

    }



    if (s->v_max == 1 && s->h_max == 1 && s->lossless==1 && nb_components==3)

        s->rgb = 1;



    /* if different size, realloc/alloc picture */

    /* XXX: also check h_count and v_count */

    if (width != s->width || height != s->height) {

        av_freep(&s->qscale_table);



        s->width      = width;

        s->height     = height;

        s->interlaced = 0;



        /* test interlaced mode */

        if (s->first_picture   &&

            s->org_height != 0 &&

            s->height < ((s->org_height * 3) / 4)) {

            s->interlaced                    = 1;

            s->bottom_field                  = s->interlace_polarity;

            s->picture_ptr->interlaced_frame = 1;

            s->picture_ptr->top_field_first  = !s->interlace_polarity;

            height *= 2;

        }



        avcodec_set_dimensions(s->avctx, width, height);



        s->qscale_table  = av_mallocz((s->width + 15) / 16);

        s->first_picture = 0;

    }



    if (s->interlaced && (s->bottom_field == !s->interlace_polarity)) {

        if (s->progressive) {

            av_log_ask_for_sample(s->avctx, "progressively coded interlaced pictures not supported\n");

            return AVERROR_INVALIDDATA;

        }

    } else{

    /* XXX: not complete test ! */

    pix_fmt_id = (s->h_count[0] << 28) | (s->v_count[0] << 24) |

                 (s->h_count[1] << 20) | (s->v_count[1] << 16) |

                 (s->h_count[2] << 12) | (s->v_count[2] <<  8) |

                 (s->h_count[3] <<  4) |  s->v_count[3];

    av_log(s->avctx, AV_LOG_DEBUG, "pix fmt id %x\n", pix_fmt_id);

    /* NOTE we do not allocate pictures large enough for the possible

     * padding of h/v_count being 4 */

    if (!(pix_fmt_id & 0xD0D0D0D0))

        pix_fmt_id -= (pix_fmt_id & 0xF0F0F0F0) >> 1;

    if (!(pix_fmt_id & 0x0D0D0D0D))

        pix_fmt_id -= (pix_fmt_id & 0x0F0F0F0F) >> 1;



    switch (pix_fmt_id) {

    case 0x11111100:

        if (s->rgb)

            s->avctx->pix_fmt = AV_PIX_FMT_BGR24;

        else {

            if (s->component_id[0] == 'Q' && s->component_id[1] == 'F' && s->component_id[2] == 'A') {

                s->avctx->pix_fmt = AV_PIX_FMT_GBR24P;

            } else {

            s->avctx->pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV444P : AV_PIX_FMT_YUVJ444P;

            s->avctx->color_range = s->cs_itu601 ? AVCOL_RANGE_MPEG : AVCOL_RANGE_JPEG;

            }

        }

        av_assert0(s->nb_components == 3);

        break;

    case 0x12121100:

    case 0x22122100:

        s->avctx->pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV444P : AV_PIX_FMT_YUVJ444P;

        s->avctx->color_range = s->cs_itu601 ? AVCOL_RANGE_MPEG : AVCOL_RANGE_JPEG;

        s->upscale_v = 2;

        s->upscale_h = (pix_fmt_id == 0x22122100);

        s->chroma_height = s->height;

        break;

    case 0x21211100:

    case 0x22211200:

        s->avctx->pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV444P : AV_PIX_FMT_YUVJ444P;

        s->avctx->color_range = s->cs_itu601 ? AVCOL_RANGE_MPEG : AVCOL_RANGE_JPEG;

        s->upscale_v = (pix_fmt_id == 0x22211200);

        s->upscale_h = 2;

        s->chroma_height = s->height;

        break;

    case 0x22221100:

        s->avctx->pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV444P : AV_PIX_FMT_YUVJ444P;

        s->avctx->color_range = s->cs_itu601 ? AVCOL_RANGE_MPEG : AVCOL_RANGE_JPEG;

        s->upscale_v = 2;

        s->upscale_h = 2;

        s->chroma_height = s->height / 2;

        break;

    case 0x11000000:

        if(s->bits <= 8)

            s->avctx->pix_fmt = AV_PIX_FMT_GRAY8;

        else

            s->avctx->pix_fmt = AV_PIX_FMT_GRAY16;

        break;

    case 0x12111100:

    case 0x22211100:

    case 0x22112100:

        s->avctx->pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV440P : AV_PIX_FMT_YUVJ440P;

        s->avctx->color_range = s->cs_itu601 ? AVCOL_RANGE_MPEG : AVCOL_RANGE_JPEG;

        s->upscale_h = (pix_fmt_id == 0x22211100) * 2 + (pix_fmt_id == 0x22112100);

        s->chroma_height = s->height / 2;

        break;

    case 0x21111100:

        s->avctx->pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV422P : AV_PIX_FMT_YUVJ422P;

        s->avctx->color_range = s->cs_itu601 ? AVCOL_RANGE_MPEG : AVCOL_RANGE_JPEG;

        break;

    case 0x22121100:

    case 0x22111200:

        s->avctx->pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV422P : AV_PIX_FMT_YUVJ422P;

        s->avctx->color_range = s->cs_itu601 ? AVCOL_RANGE_MPEG : AVCOL_RANGE_JPEG;

        s->upscale_v = (pix_fmt_id == 0x22121100) + 1;

        break;

    case 0x22111100:

        s->avctx->pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV420P : AV_PIX_FMT_YUVJ420P;

        s->avctx->color_range = s->cs_itu601 ? AVCOL_RANGE_MPEG : AVCOL_RANGE_JPEG;

        break;

    default:

        av_log(s->avctx, AV_LOG_ERROR, "Unhandled pixel format 0x%x\n", pix_fmt_id);

        return AVERROR_PATCHWELCOME;

    }

    if ((s->upscale_h || s->upscale_v) && s->avctx->lowres) {

        av_log(s->avctx, AV_LOG_ERROR, "lowres not supported for weird subsampling\n");

        return AVERROR_PATCHWELCOME;

    }

    if (s->ls) {

        s->upscale_h = s->upscale_v = 0;

        if (s->nb_components > 1)

            s->avctx->pix_fmt = AV_PIX_FMT_RGB24;

        else if (s->bits <= 8)

            s->avctx->pix_fmt = AV_PIX_FMT_GRAY8;

        else

            s->avctx->pix_fmt = AV_PIX_FMT_GRAY16;

    }



    if (s->picture_ptr->data[0])

        s->avctx->release_buffer(s->avctx, s->picture_ptr);



    if (s->avctx->get_buffer(s->avctx, s->picture_ptr) < 0) {

        av_log(s->avctx, AV_LOG_ERROR, "get_buffer() failed\n");

        return -1;

    }

    s->picture_ptr->pict_type = AV_PICTURE_TYPE_I;

    s->picture_ptr->key_frame = 1;

    s->got_picture            = 1;



    for (i = 0; i < 3; i++)

        s->linesize[i] = s->picture_ptr->linesize[i] << s->interlaced;



    av_dlog(s->avctx, "%d %d %d %d %d %d\n",

            s->width, s->height, s->linesize[0], s->linesize[1],

            s->interlaced, s->avctx->height);



    if (len != (8 + (3 * nb_components)))

        av_log(s->avctx, AV_LOG_DEBUG, "decode_sof0: error, len(%d) mismatch\n", len);

    }



    /* totally blank picture as progressive JPEG will only add details to it */

    if (s->progressive) {

        int bw = (width  + s->h_max * 8 - 1) / (s->h_max * 8);

        int bh = (height + s->v_max * 8 - 1) / (s->v_max * 8);

        for (i = 0; i < s->nb_components; i++) {

            int size = bw * bh * s->h_count[i] * s->v_count[i];

            av_freep(&s->blocks[i]);

            av_freep(&s->last_nnz[i]);

            s->blocks[i]       = av_malloc(size * sizeof(**s->blocks));

            s->last_nnz[i]     = av_mallocz(size * sizeof(**s->last_nnz));

            s->block_stride[i] = bw * s->h_count[i];

        }

        memset(s->coefs_finished, 0, sizeof(s->coefs_finished));

    }

    return 0;

}
