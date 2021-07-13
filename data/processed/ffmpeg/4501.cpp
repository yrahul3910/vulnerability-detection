static void sdp_parse_fmtp_config(AVCodecContext * codec, void *ctx,

                                  char *attr, char *value)

{

    switch (codec->codec_id) {

        case CODEC_ID_MPEG4:

        case CODEC_ID_AAC:

            if (!strcmp(attr, "config")) {

                /* decode the hexa encoded parameter */

                int len = hex_to_data(NULL, value);

                if (codec->extradata)

                    av_free(codec->extradata);

                codec->extradata = av_mallocz(len + FF_INPUT_BUFFER_PADDING_SIZE);

                if (!codec->extradata)

                    return;

                codec->extradata_size = len;

                hex_to_data(codec->extradata, value);

            }

            break;

        case CODEC_ID_VORBIS:

            ff_vorbis_parse_fmtp_config(codec, ctx, attr, value);

            break;

        default:

            break;

    }

    return;

}
