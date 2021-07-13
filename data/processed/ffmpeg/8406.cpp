static AVStream * parse_media_type(AVFormatContext *s, AVStream *st, int sid,

                                   ff_asf_guid mediatype, ff_asf_guid subtype,

                                   ff_asf_guid formattype, int size)

{

    WtvContext *wtv = s->priv_data;

    AVIOContext *pb = wtv->pb;

    if (!ff_guidcmp(subtype, ff_mediasubtype_cpfilters_processed) &&

        !ff_guidcmp(formattype, ff_format_cpfilters_processed)) {

        ff_asf_guid actual_subtype;

        ff_asf_guid actual_formattype;



        if (size < 32) {

            av_log(s, AV_LOG_WARNING, "format buffer size underflow\n");

            avio_skip(pb, size);

            return NULL;

        }



        avio_skip(pb, size - 32);

        ff_get_guid(pb, &actual_subtype);

        ff_get_guid(pb, &actual_formattype);

        avio_seek(pb, -size, SEEK_CUR);



        st = parse_media_type(s, st, sid, mediatype, actual_subtype, actual_formattype, size - 32);

        avio_skip(pb, 32);

        return st;

    } else if (!ff_guidcmp(mediatype, ff_mediatype_audio)) {

        st = new_stream(s, st, sid, AVMEDIA_TYPE_AUDIO);

        if (!st)

            return NULL;

        if (!ff_guidcmp(formattype, ff_format_waveformatex)) {

            int ret = ff_get_wav_header(pb, st->codec, size);

            if (ret < 0)

                return NULL;

        } else {

            if (ff_guidcmp(formattype, ff_format_none))

                av_log(s, AV_LOG_WARNING, "unknown formattype:"FF_PRI_GUID"\n", FF_ARG_GUID(formattype));

            avio_skip(pb, size);

        }



        if (!memcmp(subtype + 4, (const uint8_t[]){FF_MEDIASUBTYPE_BASE_GUID}, 12)) {

            st->codec->codec_id = ff_wav_codec_get_id(AV_RL32(subtype), st->codec->bits_per_coded_sample);

        } else if (!ff_guidcmp(subtype, mediasubtype_mpeg1payload)) {

            if (st->codec->extradata && st->codec->extradata_size >= 22)

                parse_mpeg1waveformatex(st);

            else

                av_log(s, AV_LOG_WARNING, "MPEG1WAVEFORMATEX underflow\n");

        } else {

            st->codec->codec_id = ff_codec_guid_get_id(ff_codec_wav_guids, subtype);

            if (st->codec->codec_id == AV_CODEC_ID_NONE)

                av_log(s, AV_LOG_WARNING, "unknown subtype:"FF_PRI_GUID"\n", FF_ARG_GUID(subtype));

        }

        return st;

    } else if (!ff_guidcmp(mediatype, ff_mediatype_video)) {

        st = new_stream(s, st, sid, AVMEDIA_TYPE_VIDEO);

        if (!st)

            return NULL;

        if (!ff_guidcmp(formattype, ff_format_videoinfo2)) {

            int consumed = parse_videoinfoheader2(s, st);

            avio_skip(pb, FFMAX(size - consumed, 0));

        } else if (!ff_guidcmp(formattype, ff_format_mpeg2_video)) {

            int consumed = parse_videoinfoheader2(s, st);

            int count;

            avio_skip(pb, 4);

            count = avio_rl32(pb);

            avio_skip(pb, 12);

            if (count && ff_get_extradata(st->codec, pb, count) < 0) {

               ff_free_stream(s, st);

               return NULL;

            }

            consumed += 20 + count;

            avio_skip(pb, FFMAX(size - consumed, 0));

        } else {

            if (ff_guidcmp(formattype, ff_format_none))

                av_log(s, AV_LOG_WARNING, "unknown formattype:"FF_PRI_GUID"\n", FF_ARG_GUID(formattype));

            avio_skip(pb, size);

        }



        if (!memcmp(subtype + 4, (const uint8_t[]){FF_MEDIASUBTYPE_BASE_GUID}, 12)) {

            st->codec->codec_id = ff_codec_get_id(ff_codec_bmp_tags, AV_RL32(subtype));

        } else {

            st->codec->codec_id = ff_codec_guid_get_id(ff_video_guids, subtype);

        }

        if (st->codec->codec_id == AV_CODEC_ID_NONE)

            av_log(s, AV_LOG_WARNING, "unknown subtype:"FF_PRI_GUID"\n", FF_ARG_GUID(subtype));

        return st;

    } else if (!ff_guidcmp(mediatype, mediatype_mpeg2_pes) &&

               !ff_guidcmp(subtype, mediasubtype_dvb_subtitle)) {

        st = new_stream(s, st, sid, AVMEDIA_TYPE_SUBTITLE);

        if (!st)

            return NULL;

        if (ff_guidcmp(formattype, ff_format_none))

            av_log(s, AV_LOG_WARNING, "unknown formattype:"FF_PRI_GUID"\n", FF_ARG_GUID(formattype));

        avio_skip(pb, size);

        st->codec->codec_id = AV_CODEC_ID_DVB_SUBTITLE;

        return st;

    } else if (!ff_guidcmp(mediatype, mediatype_mstvcaption) &&

               (!ff_guidcmp(subtype, mediasubtype_teletext) || !ff_guidcmp(subtype, mediasubtype_dtvccdata))) {

        st = new_stream(s, st, sid, AVMEDIA_TYPE_SUBTITLE);

        if (!st)

            return NULL;

        if (ff_guidcmp(formattype, ff_format_none))

            av_log(s, AV_LOG_WARNING, "unknown formattype:"FF_PRI_GUID"\n", FF_ARG_GUID(formattype));

        avio_skip(pb, size);

        st->codec->codec_id = !ff_guidcmp(subtype, mediasubtype_teletext) ? AV_CODEC_ID_DVB_TELETEXT : AV_CODEC_ID_EIA_608;

        return st;

    } else if (!ff_guidcmp(mediatype, mediatype_mpeg2_sections) &&

               !ff_guidcmp(subtype, mediasubtype_mpeg2_sections)) {

        if (ff_guidcmp(formattype, ff_format_none))

            av_log(s, AV_LOG_WARNING, "unknown formattype:"FF_PRI_GUID"\n", FF_ARG_GUID(formattype));

        avio_skip(pb, size);

        return NULL;

    }



    av_log(s, AV_LOG_WARNING, "unknown media type, mediatype:"FF_PRI_GUID

                              ", subtype:"FF_PRI_GUID", formattype:"FF_PRI_GUID"\n",

                              FF_ARG_GUID(mediatype), FF_ARG_GUID(subtype), FF_ARG_GUID(formattype));

    avio_skip(pb, size);

    return NULL;

}
