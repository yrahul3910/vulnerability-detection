static int tiff_decode_tag(TiffContext *s, const uint8_t *start, const uint8_t *buf, const uint8_t *end_buf)

{

    int tag, type, count, off, value = 0;

    int i, j;

    uint32_t *pal;

    const uint8_t *rp, *gp, *bp;



    tag = tget_short(&buf, s->le);

    type = tget_short(&buf, s->le);

    count = tget_long(&buf, s->le);

    off = tget_long(&buf, s->le);



    if(count == 1){

        switch(type){

        case TIFF_BYTE:

        case TIFF_SHORT:

            buf -= 4;

            value = tget(&buf, type, s->le);

            buf = NULL;

            break;

        case TIFF_LONG:

            value = off;

            buf = NULL;

            break;

        case TIFF_STRING:

            if(count <= 4){

                buf -= 4;

                break;

            }

        default:

            value = -1;

            buf = start + off;

        }

    }else if(type_sizes[type] * count <= 4){

        buf -= 4;

    }else{

        buf = start + off;

    }



    if(buf && (buf < start || buf > end_buf)){

        av_log(s->avctx, AV_LOG_ERROR, "Tag referencing position outside the image\n");

        return -1;

    }



    switch(tag){

    case TIFF_WIDTH:

        s->width = value;

        break;

    case TIFF_HEIGHT:

        s->height = value;

        break;

    case TIFF_BPP:

        if(count == 1) s->bpp = value;

        else{

            switch(type){

            case TIFF_BYTE:

                s->bpp = (off & 0xFF) + ((off >> 8) & 0xFF) + ((off >> 16) & 0xFF) + ((off >> 24) & 0xFF);

                break;

            case TIFF_SHORT:

            case TIFF_LONG:

                s->bpp = 0;

                for(i = 0; i < count; i++) s->bpp += tget(&buf, type, s->le);

                break;

            default:

                s->bpp = -1;

            }

        }

        switch(s->bpp){

        case 1:

            s->avctx->pix_fmt = PIX_FMT_MONOBLACK;

            break;

        case 8:

            s->avctx->pix_fmt = PIX_FMT_PAL8;

            break;

        case 24:

            s->avctx->pix_fmt = PIX_FMT_RGB24;

            break;

        case 16:

            if(count == 1){

                s->avctx->pix_fmt = PIX_FMT_GRAY16BE;

            }else{

                av_log(s->avctx, AV_LOG_ERROR, "This format is not supported (bpp=%i)\n", s->bpp);

                return -1;

            }

            break;

        case 32:

            if(count == 4){

                s->avctx->pix_fmt = PIX_FMT_RGBA;

            }else{

                av_log(s->avctx, AV_LOG_ERROR, "This format is not supported (bpp=%d, %d components)\n", s->bpp, count);

                return -1;

            }

            break;

        default:

            av_log(s->avctx, AV_LOG_ERROR, "This format is not supported (bpp=%d, %d components)\n", s->bpp, count);

            return -1;

        }

        if(s->width != s->avctx->width || s->height != s->avctx->height){

            if(avcodec_check_dimensions(s->avctx, s->width, s->height))

                return -1;

            avcodec_set_dimensions(s->avctx, s->width, s->height);

        }

        if(s->picture.data[0])

            s->avctx->release_buffer(s->avctx, &s->picture);

        if(s->avctx->get_buffer(s->avctx, &s->picture) < 0){

            av_log(s->avctx, AV_LOG_ERROR, "get_buffer() failed\n");

            return -1;

        }

        if(s->bpp == 8){

            /* make default grayscale pal */

            pal = (uint32_t *) s->picture.data[1];

            for(i = 0; i < 256; i++)

                pal[i] = i * 0x010101;

        }

        break;

    case TIFF_COMPR:

        s->compr = value;

        s->predictor = 0;

        switch(s->compr){

        case TIFF_RAW:

        case TIFF_PACKBITS:

        case TIFF_LZW:

        case TIFF_CCITT_RLE:

            break;

        case TIFF_G3:

        case TIFF_G4:

            s->fax_opts = 0;

            break;

        case TIFF_DEFLATE:

        case TIFF_ADOBE_DEFLATE:

#if CONFIG_ZLIB

            break;

#else

            av_log(s->avctx, AV_LOG_ERROR, "Deflate: ZLib not compiled in\n");

            return -1;

#endif

        case TIFF_JPEG:

        case TIFF_NEWJPEG:

            av_log(s->avctx, AV_LOG_ERROR, "JPEG compression is not supported\n");

            return -1;

        default:

            av_log(s->avctx, AV_LOG_ERROR, "Unknown compression method %i\n", s->compr);

            return -1;

        }

        break;

    case TIFF_ROWSPERSTRIP:

        if(type == TIFF_LONG && value == -1)

            value = s->avctx->height;

        if(value < 1){

            av_log(s->avctx, AV_LOG_ERROR, "Incorrect value of rows per strip\n");

            return -1;

        }

        s->rps = value;

        break;

    case TIFF_STRIP_OFFS:

        if(count == 1){

            s->stripdata = NULL;

            s->stripoff = value;

        }else

            s->stripdata = start + off;

        s->strips = count;

        if(s->strips == 1) s->rps = s->height;

        s->sot = type;

        if(s->stripdata > end_buf){

            av_log(s->avctx, AV_LOG_ERROR, "Tag referencing position outside the image\n");

            return -1;

        }

        break;

    case TIFF_STRIP_SIZE:

        if(count == 1){

            s->stripsizes = NULL;

            s->stripsize = value;

            s->strips = 1;

        }else{

            s->stripsizes = start + off;

        }

        s->strips = count;

        s->sstype = type;

        if(s->stripsizes > end_buf){

            av_log(s->avctx, AV_LOG_ERROR, "Tag referencing position outside the image\n");

            return -1;

        }

        break;

    case TIFF_PREDICTOR:

        s->predictor = value;

        break;

    case TIFF_INVERT:

        switch(value){

        case 0:

            s->invert = 1;

            break;

        case 1:

            s->invert = 0;

            break;

        case 2:

        case 3:

            break;

        default:

            av_log(s->avctx, AV_LOG_ERROR, "Color mode %d is not supported\n", value);

            return -1;

        }

        break;

    case TIFF_PAL:

        if(s->avctx->pix_fmt != PIX_FMT_PAL8){

            av_log(s->avctx, AV_LOG_ERROR, "Palette met but this is not palettized format\n");

            return -1;

        }

        pal = (uint32_t *) s->picture.data[1];

        off = type_sizes[type];

        rp = buf;

        gp = buf + count / 3 * off;

        bp = buf + count / 3 * off * 2;

        off = (type_sizes[type] - 1) << 3;

        for(i = 0; i < count / 3; i++){

            j = (tget(&rp, type, s->le) >> off) << 16;

            j |= (tget(&gp, type, s->le) >> off) << 8;

            j |= tget(&bp, type, s->le) >> off;

            pal[i] = j;

        }

        break;

    case TIFF_PLANAR:

        if(value == 2){

            av_log(s->avctx, AV_LOG_ERROR, "Planar format is not supported\n");

            return -1;

        }

        break;

    case TIFF_T4OPTIONS:

    case TIFF_T6OPTIONS:

        s->fax_opts = value;

        break;

    }

    return 0;

}
