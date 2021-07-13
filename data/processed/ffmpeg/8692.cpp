static int put_system_header(AVFormatContext *ctx, uint8_t *buf,int only_for_stream_id)

{

    MpegMuxContext *s = ctx->priv_data;

    int size, i, private_stream_coded, id;

    PutBitContext pb;



    init_put_bits(&pb, buf, 128);



    put_bits(&pb, 32, SYSTEM_HEADER_START_CODE);

    put_bits(&pb, 16, 0);

    put_bits(&pb, 1, 1);



    put_bits(&pb, 22, s->mux_rate); /* maximum bit rate of the multiplexed stream */

    put_bits(&pb, 1, 1); /* marker */

    if (s->is_vcd && only_for_stream_id==VIDEO_ID) {

        /* This header applies only to the video stream (see VCD standard p. IV-7)*/

        put_bits(&pb, 6, 0);

    } else

        put_bits(&pb, 6, s->audio_bound);



    if (s->is_vcd) {

        /* see VCD standard, p. IV-7*/

        put_bits(&pb, 1, 0);

        put_bits(&pb, 1, 1);

    } else {

        put_bits(&pb, 1, 0); /* variable bitrate*/

        put_bits(&pb, 1, 0); /* non constrainted bit stream */

    }



    if (s->is_vcd || s->is_dvd) {

        /* see VCD standard p IV-7 */

        put_bits(&pb, 1, 1); /* audio locked */

        put_bits(&pb, 1, 1); /* video locked */

    } else {

        put_bits(&pb, 1, 0); /* audio locked */

        put_bits(&pb, 1, 0); /* video locked */

    }



    put_bits(&pb, 1, 1); /* marker */



    if (s->is_vcd && only_for_stream_id==AUDIO_ID) {

        /* This header applies only to the audio stream (see VCD standard p. IV-7)*/

        put_bits(&pb, 5, 0);

    } else

        put_bits(&pb, 5, s->video_bound);



    if (s->is_dvd) {

        put_bits(&pb, 1, 0);    /* packet_rate_restriction_flag */

        put_bits(&pb, 7, 0x7f); /* reserved byte */

    } else

        put_bits(&pb, 8, 0xff); /* reserved byte */



    /* DVD-Video Stream_bound entries

    id (0xB9) video, maximum P-STD for stream 0xE0. (P-STD_buffer_bound_scale = 1)

    id (0xB8) audio, maximum P-STD for any MPEG audio (0xC0 to 0xC7) streams. If there are none set to 4096 (32x128). (P-STD_buffer_bound_scale = 0)

    id (0xBD) private stream 1 (audio other than MPEG and subpictures). (P-STD_buffer_bound_scale = 1)

    id (0xBF) private stream 2, NAV packs, set to 2x1024. */

    if (s->is_dvd) {



        int P_STD_max_video = 0;

        int P_STD_max_mpeg_audio = 0;

        int P_STD_max_mpeg_PS1 = 0;



        for(i=0;i<ctx->nb_streams;i++) {

            StreamInfo *stream = ctx->streams[i]->priv_data;



            id = stream->id;

            if (id == 0xbd && stream->max_buffer_size > P_STD_max_mpeg_PS1) {

                P_STD_max_mpeg_PS1 = stream->max_buffer_size;

            } else if (id >= 0xc0 && id <= 0xc7 && stream->max_buffer_size > P_STD_max_mpeg_audio) {

                P_STD_max_mpeg_audio = stream->max_buffer_size;

            } else if (id == 0xe0 && stream->max_buffer_size > P_STD_max_video) {

                P_STD_max_video = stream->max_buffer_size;

            }

        }



        /* video */

        put_bits(&pb, 8, 0xb9); /* stream ID */

        put_bits(&pb, 2, 3);

        put_bits(&pb, 1, 1);

        put_bits(&pb, 13, P_STD_max_video / 1024);



        /* audio */

        if (P_STD_max_mpeg_audio == 0)

            P_STD_max_mpeg_audio = 4096;

        put_bits(&pb, 8, 0xb8); /* stream ID */

        put_bits(&pb, 2, 3);

        put_bits(&pb, 1, 0);

        put_bits(&pb, 13, P_STD_max_mpeg_audio / 128);



        /* private stream 1 */

        put_bits(&pb, 8, 0xbd); /* stream ID */

        put_bits(&pb, 2, 3);

        put_bits(&pb, 1, 0);

        put_bits(&pb, 13, P_STD_max_mpeg_PS1 / 128);



        /* private stream 2 */

        put_bits(&pb, 8, 0xbf); /* stream ID */

        put_bits(&pb, 2, 3);

        put_bits(&pb, 1, 1);

        put_bits(&pb, 13, 2);

    }

    else {

        /* audio stream info */

        private_stream_coded = 0;

        for(i=0;i<ctx->nb_streams;i++) {

            StreamInfo *stream = ctx->streams[i]->priv_data;





            /* For VCDs, only include the stream info for the stream

            that the pack which contains this system belongs to.

            (see VCD standard p. IV-7) */

            if ( !s->is_vcd || stream->id==only_for_stream_id

                || only_for_stream_id==0) {



                id = stream->id;

                if (id < 0xc0) {

                    /* special case for private streams (AC-3 uses that) */

                    if (private_stream_coded)

                        continue;

                    private_stream_coded = 1;

                    id = 0xbd;

                }

                put_bits(&pb, 8, id); /* stream ID */

                put_bits(&pb, 2, 3);

                if (id < 0xe0) {

                    /* audio */

                    put_bits(&pb, 1, 0);

                    put_bits(&pb, 13, stream->max_buffer_size / 128);

                } else {

                    /* video */

                    put_bits(&pb, 1, 1);

                    put_bits(&pb, 13, stream->max_buffer_size / 1024);

                }

            }

        }

    }



    flush_put_bits(&pb);

    size = put_bits_ptr(&pb) - pb.buf;

    /* patch packet size */

    buf[4] = (size - 6) >> 8;

    buf[5] = (size - 6) & 0xff;



    return size;

}
