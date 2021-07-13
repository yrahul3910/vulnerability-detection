static int tak_read_header(AVFormatContext *s)

{

    TAKDemuxContext *tc = s->priv_data;

    AVIOContext *pb     = s->pb;

    GetBitContext gb;

    AVStream *st;

    uint8_t *buffer = NULL;

    int ret;



    st = avformat_new_stream(s, 0);

    if (!st)

        return AVERROR(ENOMEM);



    st->codec->codec_type = AVMEDIA_TYPE_AUDIO;

    st->codec->codec_id   = AV_CODEC_ID_TAK;

    st->need_parsing      = AVSTREAM_PARSE_FULL_RAW;



    tc->mlast_frame = 0;

    if (avio_rl32(pb) != MKTAG('t', 'B', 'a', 'K')) {

        avio_seek(pb, -4, SEEK_CUR);

        return 0;

    }



    while (!url_feof(pb)) {

        enum TAKMetaDataType type;

        int size;



        type = avio_r8(pb) & 0x7f;

        size = avio_rl24(pb);



        switch (type) {

        case TAK_METADATA_STREAMINFO:

        case TAK_METADATA_LAST_FRAME:

        case TAK_METADATA_ENCODER:

            buffer = av_malloc(size + FF_INPUT_BUFFER_PADDING_SIZE);

            if (!buffer)

                return AVERROR(ENOMEM);



            if (avio_read(pb, buffer, size) != size) {

                av_freep(&buffer);

                return AVERROR(EIO);

            }



            init_get_bits(&gb, buffer, size * 8);

            break;

        case TAK_METADATA_MD5: {

            uint8_t md5[16];

            int i;



            if (size != 19)

                return AVERROR_INVALIDDATA;

            avio_read(pb, md5, 16);

            avio_skip(pb, 3);

            av_log(s, AV_LOG_VERBOSE, "MD5=");

            for (i = 0; i < 16; i++)

                av_log(s, AV_LOG_VERBOSE, "%02x", md5[i]);

            av_log(s, AV_LOG_VERBOSE, "\n");

            break;

        }

        case TAK_METADATA_END: {

            int64_t curpos = avio_tell(pb);



            if (pb->seekable) {

                ff_ape_parse_tag(s);

                avio_seek(pb, curpos, SEEK_SET);

            }



            tc->data_end += curpos;

            return 0;

        }

        default:

            ret = avio_skip(pb, size);

            if (ret < 0)

                return ret;

        }



        if (type == TAK_METADATA_STREAMINFO) {

            TAKStreamInfo ti;



            avpriv_tak_parse_streaminfo(&gb, &ti);

            if (ti.samples > 0)

                st->duration = ti.samples;

            st->codec->bits_per_coded_sample = ti.bps;

            if (ti.ch_layout)

                st->codec->channel_layout = ti.ch_layout;

            st->codec->sample_rate           = ti.sample_rate;

            st->codec->channels              = ti.channels;

            st->start_time                   = 0;

            avpriv_set_pts_info(st, 64, 1, st->codec->sample_rate);

            st->codec->extradata             = buffer;

            st->codec->extradata_size        = size;

            buffer                           = NULL;

        } else if (type == TAK_METADATA_LAST_FRAME) {

            if (size != 11)

                return AVERROR_INVALIDDATA;

            tc->mlast_frame = 1;

            tc->data_end    = get_bits64(&gb, TAK_LAST_FRAME_POS_BITS) +

                              get_bits(&gb, TAK_LAST_FRAME_SIZE_BITS);

            av_freep(&buffer);

        } else if (type == TAK_METADATA_ENCODER) {

            av_log(s, AV_LOG_VERBOSE, "encoder version: %0X\n",

                   get_bits_long(&gb, TAK_ENCODER_VERSION_BITS));

            av_freep(&buffer);

        }

    }



    return AVERROR_EOF;

}
