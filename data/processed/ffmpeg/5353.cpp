int attribute_align_arg avcodec_open2(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options)

{

    int ret = 0;

    AVDictionary *tmp = NULL;



    if (avcodec_is_open(avctx))

        return 0;



    if ((!codec && !avctx->codec)) {

        av_log(avctx, AV_LOG_ERROR, "No codec provided to avcodec_open2()\n");

        return AVERROR(EINVAL);

    }

    if ((codec && avctx->codec && codec != avctx->codec)) {

        av_log(avctx, AV_LOG_ERROR, "This AVCodecContext was allocated for %s, "

                                    "but %s passed to avcodec_open2()\n", avctx->codec->name, codec->name);

        return AVERROR(EINVAL);

    }

    if (!codec)

        codec = avctx->codec;



    if (avctx->extradata_size < 0 || avctx->extradata_size >= FF_MAX_EXTRADATA_SIZE)

        return AVERROR(EINVAL);



    if (options)

        av_dict_copy(&tmp, *options, 0);



    ret = ff_lock_avcodec(avctx);

    if (ret < 0)

        return ret;



    avctx->internal = av_mallocz(sizeof(AVCodecInternal));

    if (!avctx->internal) {

        ret = AVERROR(ENOMEM);

        goto end;

    }



    avctx->internal->pool = av_mallocz(sizeof(*avctx->internal->pool));

    if (!avctx->internal->pool) {

        ret = AVERROR(ENOMEM);

        goto free_and_end;

    }



    if (codec->priv_data_size > 0) {

        if (!avctx->priv_data) {

            avctx->priv_data = av_mallocz(codec->priv_data_size);

            if (!avctx->priv_data) {

                ret = AVERROR(ENOMEM);

                goto end;

            }

            if (codec->priv_class) {

                *(const AVClass **)avctx->priv_data = codec->priv_class;

                av_opt_set_defaults(avctx->priv_data);

            }

        }

        if (codec->priv_class && (ret = av_opt_set_dict(avctx->priv_data, &tmp)) < 0)

            goto free_and_end;

    } else {

        avctx->priv_data = NULL;

    }

    if ((ret = av_opt_set_dict(avctx, &tmp)) < 0)

        goto free_and_end;



    //We only call avcodec_set_dimensions() for non h264 codecs so as not to overwrite previously setup dimensions

    if (!( avctx->coded_width && avctx->coded_height && avctx->width && avctx->height && avctx->codec_id == AV_CODEC_ID_H264)){



    if (avctx->coded_width && avctx->coded_height)

        avcodec_set_dimensions(avctx, avctx->coded_width, avctx->coded_height);

    else if (avctx->width && avctx->height)

        avcodec_set_dimensions(avctx, avctx->width, avctx->height);

    }



    if ((avctx->coded_width || avctx->coded_height || avctx->width || avctx->height)

        && (  av_image_check_size(avctx->coded_width, avctx->coded_height, 0, avctx) < 0

           || av_image_check_size(avctx->width,       avctx->height,       0, avctx) < 0)) {

        av_log(avctx, AV_LOG_WARNING, "Ignoring invalid width/height values\n");

        avcodec_set_dimensions(avctx, 0, 0);

    }



    /* if the decoder init function was already called previously,

     * free the already allocated subtitle_header before overwriting it */

    if (av_codec_is_decoder(codec))

        av_freep(&avctx->subtitle_header);



    if (avctx->channels > FF_SANE_NB_CHANNELS) {

        ret = AVERROR(EINVAL);

        goto free_and_end;

    }



    avctx->codec = codec;

    if ((avctx->codec_type == AVMEDIA_TYPE_UNKNOWN || avctx->codec_type == codec->type) &&

        avctx->codec_id == AV_CODEC_ID_NONE) {

        avctx->codec_type = codec->type;

        avctx->codec_id   = codec->id;

    }

    if (avctx->codec_id != codec->id || (avctx->codec_type != codec->type

                                         && avctx->codec_type != AVMEDIA_TYPE_ATTACHMENT)) {

        av_log(avctx, AV_LOG_ERROR, "Codec type or id mismatches\n");

        ret = AVERROR(EINVAL);

        goto free_and_end;

    }

    avctx->frame_number = 0;

    avctx->codec_descriptor = avcodec_descriptor_get(avctx->codec_id);



    if (avctx->codec->capabilities & CODEC_CAP_EXPERIMENTAL &&

        avctx->strict_std_compliance > FF_COMPLIANCE_EXPERIMENTAL) {

        const char *codec_string = av_codec_is_encoder(codec) ? "encoder" : "decoder";

        AVCodec *codec2;

        av_log(NULL, AV_LOG_ERROR,

               "The %s '%s' is experimental but experimental codecs are not enabled, "

               "add '-strict %d' if you want to use it.\n",

               codec_string, codec->name, FF_COMPLIANCE_EXPERIMENTAL);

        codec2 = av_codec_is_encoder(codec) ? avcodec_find_encoder(codec->id) : avcodec_find_decoder(codec->id);

        if (!(codec2->capabilities & CODEC_CAP_EXPERIMENTAL))

            av_log(NULL, AV_LOG_ERROR, "Alternatively use the non experimental %s '%s'.\n",

                codec_string, codec2->name);

        ret = AVERROR_EXPERIMENTAL;

        goto free_and_end;

    }



    if (avctx->codec_type == AVMEDIA_TYPE_AUDIO &&

        (!avctx->time_base.num || !avctx->time_base.den)) {

        avctx->time_base.num = 1;

        avctx->time_base.den = avctx->sample_rate;

    }



    if (!HAVE_THREADS)

        av_log(avctx, AV_LOG_WARNING, "Warning: not compiled with thread support, using thread emulation\n");



    if (HAVE_THREADS) {

        ff_unlock_avcodec(); //we will instanciate a few encoders thus kick the counter to prevent false detection of a problem

        ret = ff_frame_thread_encoder_init(avctx, options ? *options : NULL);

        ff_lock_avcodec(avctx);

        if (ret < 0)

            goto free_and_end;

    }



    if (HAVE_THREADS && !avctx->thread_opaque

        && !(avctx->internal->frame_thread_encoder && (avctx->active_thread_type&FF_THREAD_FRAME))) {

        ret = ff_thread_init(avctx);

        if (ret < 0) {

            goto free_and_end;

        }

    }

    if (!HAVE_THREADS && !(codec->capabilities & CODEC_CAP_AUTO_THREADS))

        avctx->thread_count = 1;



    if (avctx->codec->max_lowres < avctx->lowres || avctx->lowres < 0) {

        av_log(avctx, AV_LOG_ERROR, "The maximum value for lowres supported by the decoder is %d\n",

               avctx->codec->max_lowres);

        ret = AVERROR(EINVAL);

        goto free_and_end;

    }



    if (av_codec_is_encoder(avctx->codec)) {

        int i;

        if (avctx->codec->sample_fmts) {

            for (i = 0; avctx->codec->sample_fmts[i] != AV_SAMPLE_FMT_NONE; i++) {

                if (avctx->sample_fmt == avctx->codec->sample_fmts[i])

                    break;

                if (avctx->channels == 1 &&

                    av_get_planar_sample_fmt(avctx->sample_fmt) ==

                    av_get_planar_sample_fmt(avctx->codec->sample_fmts[i])) {

                    avctx->sample_fmt = avctx->codec->sample_fmts[i];

                    break;

                }

            }

            if (avctx->codec->sample_fmts[i] == AV_SAMPLE_FMT_NONE) {

                char buf[128];

                snprintf(buf, sizeof(buf), "%d", avctx->sample_fmt);

                av_log(avctx, AV_LOG_ERROR, "Specified sample format %s is invalid or not supported\n",

                       (char *)av_x_if_null(av_get_sample_fmt_name(avctx->sample_fmt), buf));

                ret = AVERROR(EINVAL);

                goto free_and_end;

            }

        }

        if (avctx->codec->pix_fmts) {

            for (i = 0; avctx->codec->pix_fmts[i] != AV_PIX_FMT_NONE; i++)

                if (avctx->pix_fmt == avctx->codec->pix_fmts[i])

                    break;

            if (avctx->codec->pix_fmts[i] == AV_PIX_FMT_NONE

                && !((avctx->codec_id == AV_CODEC_ID_MJPEG || avctx->codec_id == AV_CODEC_ID_LJPEG)

                     && avctx->strict_std_compliance <= FF_COMPLIANCE_UNOFFICIAL)) {

                char buf[128];

                snprintf(buf, sizeof(buf), "%d", avctx->pix_fmt);

                av_log(avctx, AV_LOG_ERROR, "Specified pixel format %s is invalid or not supported\n",

                       (char *)av_x_if_null(av_get_pix_fmt_name(avctx->pix_fmt), buf));

                ret = AVERROR(EINVAL);

                goto free_and_end;

            }

        }

        if (avctx->codec->supported_samplerates) {

            for (i = 0; avctx->codec->supported_samplerates[i] != 0; i++)

                if (avctx->sample_rate == avctx->codec->supported_samplerates[i])

                    break;

            if (avctx->codec->supported_samplerates[i] == 0) {

                av_log(avctx, AV_LOG_ERROR, "Specified sample rate %d is not supported\n",

                       avctx->sample_rate);

                ret = AVERROR(EINVAL);

                goto free_and_end;

            }

        }

        if (avctx->codec->channel_layouts) {

            if (!avctx->channel_layout) {

                av_log(avctx, AV_LOG_WARNING, "Channel layout not specified\n");

            } else {

                for (i = 0; avctx->codec->channel_layouts[i] != 0; i++)

                    if (avctx->channel_layout == avctx->codec->channel_layouts[i])

                        break;

                if (avctx->codec->channel_layouts[i] == 0) {

                    char buf[512];

                    av_get_channel_layout_string(buf, sizeof(buf), -1, avctx->channel_layout);

                    av_log(avctx, AV_LOG_ERROR, "Specified channel layout '%s' is not supported\n", buf);

                    ret = AVERROR(EINVAL);

                    goto free_and_end;

                }

            }

        }

        if (avctx->channel_layout && avctx->channels) {

            int channels = av_get_channel_layout_nb_channels(avctx->channel_layout);

            if (channels != avctx->channels) {

                char buf[512];

                av_get_channel_layout_string(buf, sizeof(buf), -1, avctx->channel_layout);

                av_log(avctx, AV_LOG_ERROR,

                       "Channel layout '%s' with %d channels does not match number of specified channels %d\n",

                       buf, channels, avctx->channels);

                ret = AVERROR(EINVAL);

                goto free_and_end;

            }

        } else if (avctx->channel_layout) {

            avctx->channels = av_get_channel_layout_nb_channels(avctx->channel_layout);

        }

        if(avctx->codec_type == AVMEDIA_TYPE_VIDEO &&

           avctx->codec_id != AV_CODEC_ID_PNG // For mplayer

        ) {

            if (avctx->width <= 0 || avctx->height <= 0) {

                av_log(avctx, AV_LOG_ERROR, "dimensions not set\n");

                ret = AVERROR(EINVAL);

                goto free_and_end;

            }

        }

        if (   (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)

            && avctx->bit_rate>0 && avctx->bit_rate<1000) {

            av_log(avctx, AV_LOG_WARNING, "Bitrate %d is extremely low, maybe you mean %dk\n", avctx->bit_rate, avctx->bit_rate);

        }



        if (!avctx->rc_initial_buffer_occupancy)

            avctx->rc_initial_buffer_occupancy = avctx->rc_buffer_size * 3 / 4;

    }



    avctx->pts_correction_num_faulty_pts =

    avctx->pts_correction_num_faulty_dts = 0;

    avctx->pts_correction_last_pts =

    avctx->pts_correction_last_dts = INT64_MIN;



    if (   avctx->codec->init && (!(avctx->active_thread_type&FF_THREAD_FRAME)

        || avctx->internal->frame_thread_encoder)) {

        ret = avctx->codec->init(avctx);

        if (ret < 0) {

            goto free_and_end;

        }

    }



    ret=0;



    if (av_codec_is_decoder(avctx->codec)) {

        if (!avctx->bit_rate)

            avctx->bit_rate = get_bit_rate(avctx);

        /* validate channel layout from the decoder */

        if (avctx->channel_layout) {

            int channels = av_get_channel_layout_nb_channels(avctx->channel_layout);

            if (!avctx->channels)

                avctx->channels = channels;

            else if (channels != avctx->channels) {

                char buf[512];

                av_get_channel_layout_string(buf, sizeof(buf), -1, avctx->channel_layout);

                av_log(avctx, AV_LOG_WARNING,

                       "Channel layout '%s' with %d channels does not match specified number of channels %d: "

                       "ignoring specified channel layout\n",

                       buf, channels, avctx->channels);

                avctx->channel_layout = 0;

            }

        }

        if (avctx->channels && avctx->channels < 0 ||

            avctx->channels > FF_SANE_NB_CHANNELS) {

            ret = AVERROR(EINVAL);

            goto free_and_end;

        }

        if (avctx->sub_charenc) {

            if (avctx->codec_type != AVMEDIA_TYPE_SUBTITLE) {

                av_log(avctx, AV_LOG_ERROR, "Character encoding is only "

                       "supported with subtitles codecs\n");

                ret = AVERROR(EINVAL);

                goto free_and_end;

            } else if (avctx->codec_descriptor->props & AV_CODEC_PROP_BITMAP_SUB) {

                av_log(avctx, AV_LOG_WARNING, "Codec '%s' is bitmap-based, "

                       "subtitles character encoding will be ignored\n",

                       avctx->codec_descriptor->name);

                avctx->sub_charenc_mode = FF_SUB_CHARENC_MODE_DO_NOTHING;

            } else {

                /* input character encoding is set for a text based subtitle

                 * codec at this point */

                if (avctx->sub_charenc_mode == FF_SUB_CHARENC_MODE_AUTOMATIC)

                    avctx->sub_charenc_mode = FF_SUB_CHARENC_MODE_PRE_DECODER;



                if (avctx->sub_charenc_mode == FF_SUB_CHARENC_MODE_PRE_DECODER) {

#if CONFIG_ICONV

                    iconv_t cd = iconv_open("UTF-8", avctx->sub_charenc);

                    if (cd == (iconv_t)-1) {

                        av_log(avctx, AV_LOG_ERROR, "Unable to open iconv context "

                               "with input character encoding \"%s\"\n", avctx->sub_charenc);

                        ret = AVERROR(errno);

                        goto free_and_end;

                    }

                    iconv_close(cd);

#else

                    av_log(avctx, AV_LOG_ERROR, "Character encoding subtitles "

                           "conversion needs a libavcodec built with iconv support "

                           "for this codec\n");

                    ret = AVERROR(ENOSYS);

                    goto free_and_end;

#endif

                }

            }

        }

    }

end:

    ff_unlock_avcodec();

    if (options) {

        av_dict_free(options);

        *options = tmp;

    }



    return ret;

free_and_end:

    av_dict_free(&tmp);

    av_freep(&avctx->priv_data);

    if (avctx->internal)

        av_freep(&avctx->internal->pool);

    av_freep(&avctx->internal);

    avctx->codec = NULL;

    goto end;

}
