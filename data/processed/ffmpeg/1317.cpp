static int transcode_init(void)

{

    int ret = 0, i, j, k;

    AVFormatContext *oc;

    AVCodecContext *codec, *icodec;

    OutputStream *ost;

    InputStream *ist;

    char error[1024];

    int want_sdp = 1;



    /* init framerate emulation */

    for (i = 0; i < nb_input_files; i++) {

        InputFile *ifile = input_files[i];

        if (ifile->rate_emu)

            for (j = 0; j < ifile->nb_streams; j++)

                input_streams[j + ifile->ist_index]->start = av_gettime();

    }



    /* output stream init */

    for (i = 0; i < nb_output_files; i++) {

        oc = output_files[i]->ctx;

        if (!oc->nb_streams && !(oc->oformat->flags & AVFMT_NOSTREAMS)) {

            av_dump_format(oc, i, oc->filename, 1);

            av_log(NULL, AV_LOG_ERROR, "Output file #%d does not contain any stream\n", i);

            return AVERROR(EINVAL);

        }

    }



    /* init complex filtergraphs */

    for (i = 0; i < nb_filtergraphs; i++)

        if ((ret = avfilter_graph_config(filtergraphs[i]->graph, NULL)) < 0)

            return ret;



    /* for each output stream, we compute the right encoding parameters */

    for (i = 0; i < nb_output_streams; i++) {

        ost = output_streams[i];

        oc  = output_files[ost->file_index]->ctx;

        ist = get_input_stream(ost);



        if (ost->attachment_filename)

            continue;



        codec  = ost->st->codec;



        if (ist) {

            icodec = ist->st->codec;



            ost->st->disposition          = ist->st->disposition;

            codec->bits_per_raw_sample    = icodec->bits_per_raw_sample;

            codec->chroma_sample_location = icodec->chroma_sample_location;

        }



        if (ost->stream_copy) {

            uint64_t extra_size;



            av_assert0(ist && !ost->filter);



            extra_size = (uint64_t)icodec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE;



            if (extra_size > INT_MAX) {

                return AVERROR(EINVAL);

            }



            /* if stream_copy is selected, no need to decode or encode */

            codec->codec_id   = icodec->codec_id;

            codec->codec_type = icodec->codec_type;



            if (!codec->codec_tag) {

                if (!oc->oformat->codec_tag ||

                     av_codec_get_id (oc->oformat->codec_tag, icodec->codec_tag) == codec->codec_id ||

                     av_codec_get_tag(oc->oformat->codec_tag, icodec->codec_id) <= 0)

                    codec->codec_tag = icodec->codec_tag;

            }



            codec->bit_rate       = icodec->bit_rate;

            codec->rc_max_rate    = icodec->rc_max_rate;

            codec->rc_buffer_size = icodec->rc_buffer_size;

            codec->field_order    = icodec->field_order;

            codec->extradata      = av_mallocz(extra_size);

            if (!codec->extradata) {

                return AVERROR(ENOMEM);

            }

            memcpy(codec->extradata, icodec->extradata, icodec->extradata_size);

            codec->extradata_size = icodec->extradata_size;

            if (!copy_tb) {

                codec->time_base      = icodec->time_base;

                codec->time_base.num *= icodec->ticks_per_frame;

                av_reduce(&codec->time_base.num, &codec->time_base.den,

                          codec->time_base.num, codec->time_base.den, INT_MAX);

            } else

                codec->time_base = ist->st->time_base;



            switch (codec->codec_type) {

            case AVMEDIA_TYPE_AUDIO:

                if (audio_volume != 256) {

                    av_log(NULL, AV_LOG_FATAL, "-acodec copy and -vol are incompatible (frames are not decoded)\n");

                    exit_program(1);

                }

                codec->channel_layout     = icodec->channel_layout;

                codec->sample_rate        = icodec->sample_rate;

                codec->channels           = icodec->channels;

                codec->frame_size         = icodec->frame_size;

                codec->audio_service_type = icodec->audio_service_type;

                codec->block_align        = icodec->block_align;

                break;

            case AVMEDIA_TYPE_VIDEO:

                codec->pix_fmt            = icodec->pix_fmt;

                codec->width              = icodec->width;

                codec->height             = icodec->height;

                codec->has_b_frames       = icodec->has_b_frames;

                if (!codec->sample_aspect_ratio.num) {

                    codec->sample_aspect_ratio   =

                    ost->st->sample_aspect_ratio =

                        ist->st->sample_aspect_ratio.num ? ist->st->sample_aspect_ratio :

                        ist->st->codec->sample_aspect_ratio.num ?

                        ist->st->codec->sample_aspect_ratio : (AVRational){0, 1};

                }

                break;

            case AVMEDIA_TYPE_SUBTITLE:

                codec->width  = icodec->width;

                codec->height = icodec->height;

                break;

            case AVMEDIA_TYPE_DATA:

            case AVMEDIA_TYPE_ATTACHMENT:

                break;

            default:

                abort();

            }

        } else {

            if (!ost->enc) {

                /* should only happen when a default codec is not present. */

                snprintf(error, sizeof(error), "Automatic encoder selection "

                         "failed for output stream #%d:%d. Default encoder for "

                         "format %s is probably disabled. Please choose an "

                         "encoder manually.\n", ost->file_index, ost->index,

                         oc->oformat->name);

                ret = AVERROR(EINVAL);

                goto dump_format;

            }



            if (ist)

                ist->decoding_needed = 1;

            ost->encoding_needed = 1;



            switch (codec->codec_type) {

            case AVMEDIA_TYPE_AUDIO:

                ost->fifo = av_fifo_alloc(1024);

                if (!ost->fifo) {

                    return AVERROR(ENOMEM);

                }



                if (!codec->sample_rate)

                    codec->sample_rate = icodec->sample_rate;

                choose_sample_rate(ost->st, ost->enc);

                codec->time_base = (AVRational){ 1, codec->sample_rate };



                if (codec->sample_fmt == AV_SAMPLE_FMT_NONE)

                    codec->sample_fmt = icodec->sample_fmt;

                choose_sample_fmt(ost->st, ost->enc);



                if (!codec->channels)

                    codec->channels = icodec->channels;

                if (!codec->channel_layout)

                    codec->channel_layout = icodec->channel_layout;

                if (av_get_channel_layout_nb_channels(codec->channel_layout) != codec->channels)

                    codec->channel_layout = 0;



                icodec->request_channels  = codec-> channels;

                ost->resample_sample_fmt  = icodec->sample_fmt;

                ost->resample_sample_rate = icodec->sample_rate;

                ost->resample_channels    = icodec->channels;

                ost->resample_channel_layout = icodec->channel_layout;

                break;

            case AVMEDIA_TYPE_VIDEO:

                if (!ost->filter) {

                    FilterGraph *fg;

                    fg = init_simple_filtergraph(ist, ost);

                    if (configure_video_filters(fg)) {

                        av_log(NULL, AV_LOG_FATAL, "Error opening filters!\n");

                        exit(1);

                    }

                }



                /*

                 * We want CFR output if and only if one of those is true:

                 * 1) user specified output framerate with -r

                 * 2) user specified -vsync cfr

                 * 3) output format is CFR and the user didn't force vsync to

                 *    something else than CFR

                 *

                 * in such a case, set ost->frame_rate

                 */

                if (!ost->frame_rate.num && ist &&

                    (video_sync_method ==  VSYNC_CFR ||

                     (video_sync_method ==  VSYNC_AUTO &&

                      !(oc->oformat->flags & (AVFMT_NOTIMESTAMPS | AVFMT_VARIABLE_FPS))))) {

                    ost->frame_rate = ist->st->r_frame_rate.num ? ist->st->r_frame_rate : (AVRational){25, 1};

                    if (ost->enc && ost->enc->supported_framerates && !ost->force_fps) {

                        int idx = av_find_nearest_q_idx(ost->frame_rate, ost->enc->supported_framerates);

                        ost->frame_rate = ost->enc->supported_framerates[idx];

                    }

                }

                if (ost->frame_rate.num) {

                    codec->time_base = (AVRational){ost->frame_rate.den, ost->frame_rate.num};

                    video_sync_method = VSYNC_CFR;

                } else if (ist)

                    codec->time_base = ist->st->time_base;

                else

                    codec->time_base = ost->filter->filter->inputs[0]->time_base;



                codec->width  = ost->filter->filter->inputs[0]->w;

                codec->height = ost->filter->filter->inputs[0]->h;

                codec->sample_aspect_ratio = ost->st->sample_aspect_ratio =

                    ost->frame_aspect_ratio ? // overridden by the -aspect cli option

                    av_d2q(ost->frame_aspect_ratio * codec->height/codec->width, 255) :

                    ost->filter->filter->inputs[0]->sample_aspect_ratio;

                codec->pix_fmt = ost->filter->filter->inputs[0]->format;



                if (codec->width   != icodec->width  ||

                    codec->height  != icodec->height ||

                    codec->pix_fmt != icodec->pix_fmt) {

                    codec->bits_per_raw_sample = 0;

                }



                break;

            case AVMEDIA_TYPE_SUBTITLE:

                codec->time_base = (AVRational){1, 1000};

                break;

            default:

                abort();

                break;

            }

            /* two pass mode */

            if ((codec->flags & (CODEC_FLAG_PASS1 | CODEC_FLAG_PASS2))) {

                char logfilename[1024];

                FILE *f;



                snprintf(logfilename, sizeof(logfilename), "%s-%d.log",

                         pass_logfilename_prefix ? pass_logfilename_prefix : DEFAULT_PASS_LOGFILENAME_PREFIX,

                         i);

                if (!strcmp(ost->enc->name, "libx264")) {

                    av_dict_set(&ost->opts, "stats", logfilename, AV_DICT_DONT_OVERWRITE);

                } else {

                    if (codec->flags & CODEC_FLAG_PASS1) {

                        f = fopen(logfilename, "wb");

                        if (!f) {

                            av_log(NULL, AV_LOG_FATAL, "Cannot write log file '%s' for pass-1 encoding: %s\n",

                                   logfilename, strerror(errno));

                            exit_program(1);

                        }

                        ost->logfile = f;

                    } else {

                        char  *logbuffer;

                        size_t logbuffer_size;

                        if (cmdutils_read_file(logfilename, &logbuffer, &logbuffer_size) < 0) {

                            av_log(NULL, AV_LOG_FATAL, "Error reading log file '%s' for pass-2 encoding\n",

                                   logfilename);

                            exit_program(1);

                        }

                        codec->stats_in = logbuffer;

                    }

                }

            }

        }

    }



    /* open each encoder */

    for (i = 0; i < nb_output_streams; i++) {

        ost = output_streams[i];

        if (ost->encoding_needed) {

            AVCodec      *codec = ost->enc;

            AVCodecContext *dec = NULL;



            if ((ist = get_input_stream(ost)))

                dec = ist->st->codec;

            if (dec && dec->subtitle_header) {

                ost->st->codec->subtitle_header = av_malloc(dec->subtitle_header_size);

                if (!ost->st->codec->subtitle_header) {

                    ret = AVERROR(ENOMEM);

                    goto dump_format;

                }

                memcpy(ost->st->codec->subtitle_header, dec->subtitle_header, dec->subtitle_header_size);

                ost->st->codec->subtitle_header_size = dec->subtitle_header_size;

            }

            if (!av_dict_get(ost->opts, "threads", NULL, 0))

                av_dict_set(&ost->opts, "threads", "auto", 0);

            if (avcodec_open2(ost->st->codec, codec, &ost->opts) < 0) {

                snprintf(error, sizeof(error), "Error while opening encoder for output stream #%d:%d - maybe incorrect parameters such as bit_rate, rate, width or height",

                        ost->file_index, ost->index);

                ret = AVERROR(EINVAL);

                goto dump_format;

            }

            assert_codec_experimental(ost->st->codec, 1);

            assert_avoptions(ost->opts);

            if (ost->st->codec->bit_rate && ost->st->codec->bit_rate < 1000)

                av_log(NULL, AV_LOG_WARNING, "The bitrate parameter is set too low."

                                             "It takes bits/s as argument, not kbits/s\n");

            extra_size += ost->st->codec->extradata_size;



            if (ost->st->codec->me_threshold)

                input_streams[ost->source_index]->st->codec->debug |= FF_DEBUG_MV;

        }

    }



    /* init input streams */

    for (i = 0; i < nb_input_streams; i++)

        if ((ret = init_input_stream(i, error, sizeof(error))) < 0)

            goto dump_format;



    /* discard unused programs */

    for (i = 0; i < nb_input_files; i++) {

        InputFile *ifile = input_files[i];

        for (j = 0; j < ifile->ctx->nb_programs; j++) {

            AVProgram *p = ifile->ctx->programs[j];

            int discard  = AVDISCARD_ALL;



            for (k = 0; k < p->nb_stream_indexes; k++)

                if (!input_streams[ifile->ist_index + p->stream_index[k]]->discard) {

                    discard = AVDISCARD_DEFAULT;

                    break;

                }

            p->discard = discard;

        }

    }



    /* open files and write file headers */

    for (i = 0; i < nb_output_files; i++) {

        oc = output_files[i]->ctx;

        oc->interrupt_callback = int_cb;

        if (avformat_write_header(oc, &output_files[i]->opts) < 0) {

            snprintf(error, sizeof(error), "Could not write header for output file #%d (incorrect codec parameters ?)", i);

            ret = AVERROR(EINVAL);

            goto dump_format;

        }

        assert_avoptions(output_files[i]->opts);

        if (strcmp(oc->oformat->name, "rtp")) {

            want_sdp = 0;

        }

    }



 dump_format:

    /* dump the file output parameters - cannot be done before in case

       of stream copy */

    for (i = 0; i < nb_output_files; i++) {

        av_dump_format(output_files[i]->ctx, i, output_files[i]->ctx->filename, 1);

    }



    /* dump the stream mapping */

    av_log(NULL, AV_LOG_INFO, "Stream mapping:\n");

    for (i = 0; i < nb_input_streams; i++) {

        ist = input_streams[i];



        for (j = 0; j < ist->nb_filters; j++) {

            AVFilterLink *link = ist->filters[j]->filter->outputs[0];

            if (ist->filters[j]->graph->graph_desc) {

                av_log(NULL, AV_LOG_INFO, "  Stream #%d:%d (%s) -> %s",

                       ist->file_index, ist->st->index, ist->dec ? ist->dec->name : "?",

                       link->dst->filter->name);

                if (link->dst->input_count > 1)

                    av_log(NULL, AV_LOG_INFO, ":%s", link->dstpad->name);

                if (nb_filtergraphs > 1)

                    av_log(NULL, AV_LOG_INFO, " (graph %d)", ist->filters[j]->graph->index);

                av_log(NULL, AV_LOG_INFO, "\n");

            }

        }

    }



    for (i = 0; i < nb_output_streams; i++) {

        ost = output_streams[i];



        if (ost->attachment_filename) {

            /* an attached file */

            av_log(NULL, AV_LOG_INFO, "  File %s -> Stream #%d:%d\n",

                   ost->attachment_filename, ost->file_index, ost->index);

            continue;

        }



        if (ost->filter && ost->filter->graph->graph_desc) {

            /* output from a complex graph */

            AVFilterLink *link = ost->filter->filter->inputs[0];

            av_log(NULL, AV_LOG_INFO, "  %s", link->src->filter->name);

            if (link->src->output_count > 1)

                av_log(NULL, AV_LOG_INFO, ":%s", link->srcpad->name);

            if (nb_filtergraphs > 1)

                av_log(NULL, AV_LOG_INFO, " (graph %d)", ost->filter->graph->index);



            av_log(NULL, AV_LOG_INFO, " -> Stream #%d:%d (%s)\n", ost->file_index,

                   ost->index, ost->enc ? ost->enc->name : "?");

            continue;

        }



        av_log(NULL, AV_LOG_INFO, "  Stream #%d:%d -> #%d:%d",

               input_streams[ost->source_index]->file_index,

               input_streams[ost->source_index]->st->index,

               ost->file_index,

               ost->index);

        if (ost->sync_ist != input_streams[ost->source_index])

            av_log(NULL, AV_LOG_INFO, " [sync #%d:%d]",

                   ost->sync_ist->file_index,

                   ost->sync_ist->st->index);

        if (ost->stream_copy)

            av_log(NULL, AV_LOG_INFO, " (copy)");

        else

            av_log(NULL, AV_LOG_INFO, " (%s -> %s)", input_streams[ost->source_index]->dec ?

                   input_streams[ost->source_index]->dec->name : "?",

                   ost->enc ? ost->enc->name : "?");

        av_log(NULL, AV_LOG_INFO, "\n");

    }



    if (ret) {

        av_log(NULL, AV_LOG_ERROR, "%s\n", error);

        return ret;

    }



    if (want_sdp) {

        print_sdp();

    }



    return 0;

}
