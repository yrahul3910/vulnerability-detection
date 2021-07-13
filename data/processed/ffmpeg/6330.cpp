static int decode_codestream(Jpeg2000DecoderContext *s)

{

    Jpeg2000CodingStyle *codsty = s->codsty;

    Jpeg2000QuantStyle  *qntsty = s->qntsty;

    uint8_t *properties = s->properties;



    for (;;){

        int oldpos, marker, len, ret = 0;



        if (bytestream2_get_bytes_left(&s->g) < 2) {

            av_log(s->avctx, AV_LOG_ERROR, "Missing EOC\n");

            break;

        }



        marker = bytestream2_get_be16u(&s->g);

        av_dlog(s->avctx, "marker 0x%.4X at pos 0x%x\n", marker, bytestream2_tell(&s->g) - 4);

        oldpos = bytestream2_tell(&s->g);



        if (marker == JPEG2000_SOD){

            Jpeg2000Tile *tile = s->tile + s->curtileno;

            if (ret = init_tile(s, s->curtileno)) {

                av_log(s->avctx, AV_LOG_ERROR, "tile initialization failed\n");

                return ret;

            }

            if (ret = jpeg2000_decode_packets(s, tile)) {

                av_log(s->avctx, AV_LOG_ERROR, "packets decoding failed\n");

                return ret;

            }

            continue;

        }

        if (marker == JPEG2000_EOC)

            break;



        if (bytestream2_get_bytes_left(&s->g) < 2)

            return AVERROR(EINVAL);

        len = bytestream2_get_be16u(&s->g);

        switch (marker){

        case JPEG2000_SIZ:

            ret = get_siz(s);



            break;

        case JPEG2000_COC:

            ret = get_coc(s, codsty, properties);

            break;

        case JPEG2000_COD:

            ret = get_cod(s, codsty, properties);

            break;

        case JPEG2000_QCC:

            ret = get_qcc(s, len, qntsty, properties);

            break;

        case JPEG2000_QCD:

            ret = get_qcd(s, len, qntsty, properties);

            break;

        case JPEG2000_SOT:

            if (!(ret = get_sot(s))){

                codsty = s->tile[s->curtileno].codsty;

                qntsty = s->tile[s->curtileno].qntsty;

                properties = s->tile[s->curtileno].properties;

            }

            break;

        case JPEG2000_COM:

            // the comment is ignored

            bytestream2_skip(&s->g, len - 2);

            break;

        default:

            av_log(s->avctx, AV_LOG_ERROR, "unsupported marker 0x%.4X at pos 0x%x\n", marker, bytestream2_tell(&s->g) - 4);

            bytestream2_skip(&s->g, len - 2);

            break;

        }

        if (bytestream2_tell(&s->g) - oldpos != len || ret){

            av_log(s->avctx, AV_LOG_ERROR, "error during processing marker segment %.4x\n", marker);

            return ret ? ret : -1;

        }

    }

    return 0;

}