static int output_packet(AVInputStream *ist, int ist_index,

                         AVOutputStream **ost_table, int nb_ostreams,

                         const AVPacket *pkt)

{

    AVFormatContext *os;

    AVOutputStream *ost;

    uint8_t *ptr;

    int len, ret, i;

    uint8_t *data_buf;

    int data_size, got_picture;

    AVFrame picture;

    short samples[AVCODEC_MAX_AUDIO_FRAME_SIZE / 2];

    void *buffer_to_free;

    

    if (pkt && pkt->pts != AV_NOPTS_VALUE) { //FIXME seems redundant, as libavformat does this too

        ist->next_pts = ist->pts = pkt->dts;

    } else {

        ist->pts = ist->next_pts;

    }

    

    if (pkt == NULL) {

        /* EOF handling */

        ptr = NULL;

        len = 0;

        goto handle_eof;

    }



    len = pkt->size;

    ptr = pkt->data;

    while (len > 0) {

    handle_eof:

        /* decode the packet if needed */

        data_buf = NULL; /* fail safe */

        data_size = 0;

        if (ist->decoding_needed) {

            switch(ist->st->codec.codec_type) {

            case CODEC_TYPE_AUDIO:

                    /* XXX: could avoid copy if PCM 16 bits with same

                       endianness as CPU */

                ret = avcodec_decode_audio(&ist->st->codec, samples, &data_size,

                                           ptr, len);

                if (ret < 0)

                    goto fail_decode;

                ptr += ret;

                len -= ret;

                /* Some bug in mpeg audio decoder gives */

                /* data_size < 0, it seems they are overflows */

                if (data_size <= 0) {

                    /* no audio frame */

                    continue;

                }

                data_buf = (uint8_t *)samples;

                ist->next_pts += ((int64_t)AV_TIME_BASE/2 * data_size) / 

                    (ist->st->codec.sample_rate * ist->st->codec.channels);

                break;

            case CODEC_TYPE_VIDEO:

                    data_size = (ist->st->codec.width * ist->st->codec.height * 3) / 2;

                    /* XXX: allocate picture correctly */

                    avcodec_get_frame_defaults(&picture);



                    ret = avcodec_decode_video(&ist->st->codec, 

                                               &picture, &got_picture, ptr, len);

                    ist->st->quality= picture.quality;

                    if (ret < 0) 

                        goto fail_decode;

                    if (!got_picture) {

                        /* no picture yet */

                        goto discard_packet;

                    }

                    if (ist->st->codec.frame_rate_base != 0) {

                        ist->next_pts += ((int64_t)AV_TIME_BASE * 

                                          ist->st->codec.frame_rate_base) /

                            ist->st->codec.frame_rate;

                    }

                    len = 0;

                    break;

                default:

                    goto fail_decode;

                }

            } else {

                data_buf = ptr;

                data_size = len;

                ret = len;

                len = 0;

            }



            buffer_to_free = NULL;

            if (ist->st->codec.codec_type == CODEC_TYPE_VIDEO) {

                pre_process_video_frame(ist, (AVPicture *)&picture, 

                                        &buffer_to_free);

            }



            /* frame rate emulation */

            if (ist->st->codec.rate_emu) {

                int64_t pts = av_rescale((int64_t) ist->frame * ist->st->codec.frame_rate_base, 1000000, ist->st->codec.frame_rate);

                int64_t now = av_gettime() - ist->start;

                if (pts > now)

                    usleep(pts - now);



                ist->frame++;

            }



#if 0

            /* mpeg PTS deordering : if it is a P or I frame, the PTS

               is the one of the next displayed one */

            /* XXX: add mpeg4 too ? */

            if (ist->st->codec.codec_id == CODEC_ID_MPEG1VIDEO) {

                if (ist->st->codec.pict_type != B_TYPE) {

                    int64_t tmp;

                    tmp = ist->last_ip_pts;

                    ist->last_ip_pts  = ist->frac_pts.val;

                    ist->frac_pts.val = tmp;

                }

            }

#endif

            /* if output time reached then transcode raw format, 

	       encode packets and output them */

            if (start_time == 0 || ist->pts >= start_time)

                for(i=0;i<nb_ostreams;i++) {

                    int frame_size;



                    ost = ost_table[i];

                    if (ost->source_index == ist_index) {

                        os = output_files[ost->file_index];



#if 0

                        printf("%d: got pts=%0.3f %0.3f\n", i, 

                               (double)pkt->pts / AV_TIME_BASE, 

                               ((double)ist->pts / AV_TIME_BASE) - 

                               ((double)ost->st->pts.val * ost->time_base.num / ost->time_base.den));

#endif

                        /* set the input output pts pairs */

                        ost->sync_ipts = (double)ist->pts / AV_TIME_BASE;



                        if (ost->encoding_needed) {

                            switch(ost->st->codec.codec_type) {

                            case CODEC_TYPE_AUDIO:

                                do_audio_out(os, ost, ist, data_buf, data_size);

                                break;

                            case CODEC_TYPE_VIDEO:

                                /* find an audio stream for synchro */

                                {

                                    int i;

                                    AVOutputStream *audio_sync, *ost1;

                                    audio_sync = NULL;

                                    for(i=0;i<nb_ostreams;i++) {

                                        ost1 = ost_table[i];

                                        if (ost1->file_index == ost->file_index &&

                                            ost1->st->codec.codec_type == CODEC_TYPE_AUDIO) {

                                            audio_sync = ost1;

                                            break;

                                        }

                                    }



                                    do_video_out(os, ost, ist, &picture, &frame_size, audio_sync);

                                    video_size += frame_size;

                                    if (do_vstats && frame_size)

                                        do_video_stats(os, ost, frame_size);

                                }

                                break;

                            default:

                                av_abort();

                            }

                        } else {

                            AVFrame avframe; //FIXME/XXX remove this

                            AVPacket opkt;

                            av_init_packet(&opkt);



                            /* no reencoding needed : output the packet directly */

                            /* force the input stream PTS */

                        

                            avcodec_get_frame_defaults(&avframe);

                            ost->st->codec.coded_frame= &avframe;

                            avframe.key_frame = pkt->flags & PKT_FLAG_KEY; 



                            if(ost->st->codec.codec_type == CODEC_TYPE_AUDIO)

                                audio_size += data_size;

                            else if (ost->st->codec.codec_type == CODEC_TYPE_VIDEO)

                                video_size += data_size;



                            opkt.stream_index= ost->index;

                            opkt.data= data_buf;

                            opkt.size= data_size;

                            opkt.pts= ist->pts; //FIXME dts vs. pts

                            opkt.flags= pkt->flags;

                            

                            av_write_frame(os, &opkt);

                            ost->st->codec.frame_number++;

                            ost->frame_number++;

                        }

                    }

                }

            av_free(buffer_to_free);

        }

 discard_packet:

    return 0;

 fail_decode:

    return -1;

}
