static int process_ipmovie_chunk(IPMVEContext *s, AVIOContext *pb,

    AVPacket *pkt)

{

    unsigned char chunk_preamble[CHUNK_PREAMBLE_SIZE];

    int chunk_type;

    int chunk_size;

    unsigned char opcode_preamble[OPCODE_PREAMBLE_SIZE];

    unsigned char opcode_type;

    unsigned char opcode_version;

    int opcode_size;

    unsigned char scratch[1024];

    int i, j;

    int first_color, last_color;

    int audio_flags;

    unsigned char r, g, b;

    unsigned int width, height;



    /* see if there are any pending packets */

    chunk_type = load_ipmovie_packet(s, pb, pkt);

    if (chunk_type != CHUNK_DONE)

        return chunk_type;



    /* read the next chunk, wherever the file happens to be pointing */

    if (avio_feof(pb))

        return CHUNK_EOF;

    if (avio_read(pb, chunk_preamble, CHUNK_PREAMBLE_SIZE) !=

        CHUNK_PREAMBLE_SIZE)

        return CHUNK_BAD;

    chunk_size = AV_RL16(&chunk_preamble[0]);

    chunk_type = AV_RL16(&chunk_preamble[2]);



    av_log(s->avf, AV_LOG_TRACE, "chunk type 0x%04X, 0x%04X bytes: ", chunk_type, chunk_size);



    switch (chunk_type) {



    case CHUNK_INIT_AUDIO:

        av_log(s->avf, AV_LOG_TRACE, "initialize audio\n");

        break;



    case CHUNK_AUDIO_ONLY:

        av_log(s->avf, AV_LOG_TRACE, "audio only\n");

        break;



    case CHUNK_INIT_VIDEO:

        av_log(s->avf, AV_LOG_TRACE, "initialize video\n");

        break;



    case CHUNK_VIDEO:

        av_log(s->avf, AV_LOG_TRACE, "video (and audio)\n");

        break;



    case CHUNK_SHUTDOWN:

        av_log(s->avf, AV_LOG_TRACE, "shutdown\n");

        break;



    case CHUNK_END:

        av_log(s->avf, AV_LOG_TRACE, "end\n");

        break;



    default:

        av_log(s->avf, AV_LOG_TRACE, "invalid chunk\n");

        chunk_type = CHUNK_BAD;

        break;



    }



    while ((chunk_size > 0) && (chunk_type != CHUNK_BAD)) {



        /* read the next chunk, wherever the file happens to be pointing */

        if (avio_feof(pb)) {

            chunk_type = CHUNK_EOF;

            break;

        }

        if (avio_read(pb, opcode_preamble, CHUNK_PREAMBLE_SIZE) !=

            CHUNK_PREAMBLE_SIZE) {

            chunk_type = CHUNK_BAD;

            break;

        }



        opcode_size = AV_RL16(&opcode_preamble[0]);

        opcode_type = opcode_preamble[2];

        opcode_version = opcode_preamble[3];



        chunk_size -= OPCODE_PREAMBLE_SIZE;

        chunk_size -= opcode_size;

        if (chunk_size < 0) {

            av_log(s->avf, AV_LOG_TRACE, "chunk_size countdown just went negative\n");

            chunk_type = CHUNK_BAD;

            break;

        }



        av_log(s->avf, AV_LOG_TRACE, "  opcode type %02X, version %d, 0x%04X bytes: ",

                opcode_type, opcode_version, opcode_size);

        switch (opcode_type) {



        case OPCODE_END_OF_STREAM:

            av_log(s->avf, AV_LOG_TRACE, "end of stream\n");

            avio_skip(pb, opcode_size);

            break;



        case OPCODE_END_OF_CHUNK:

            av_log(s->avf, AV_LOG_TRACE, "end of chunk\n");

            avio_skip(pb, opcode_size);

            break;



        case OPCODE_CREATE_TIMER:

            av_log(s->avf, AV_LOG_TRACE, "create timer\n");

            if ((opcode_version > 0) || (opcode_size != 6)) {

                av_log(s->avf, AV_LOG_TRACE, "bad create_timer opcode\n");

                chunk_type = CHUNK_BAD;

                break;

            }

            if (avio_read(pb, scratch, opcode_size) !=

                opcode_size) {

                chunk_type = CHUNK_BAD;

                break;

            }

            s->frame_pts_inc = ((uint64_t)AV_RL32(&scratch[0])) * AV_RL16(&scratch[4]);

            break;



        case OPCODE_INIT_AUDIO_BUFFERS:

            av_log(s->avf, AV_LOG_TRACE, "initialize audio buffers\n");

            if (opcode_version > 1 || opcode_size > 10 || opcode_size < 6) {

                av_log(s->avf, AV_LOG_TRACE, "bad init_audio_buffers opcode\n");

                chunk_type = CHUNK_BAD;

                break;

            }

            if (avio_read(pb, scratch, opcode_size) !=

                opcode_size) {

                chunk_type = CHUNK_BAD;

                break;

            }

            s->audio_sample_rate = AV_RL16(&scratch[4]);

            audio_flags = AV_RL16(&scratch[2]);

            /* bit 0 of the flags: 0 = mono, 1 = stereo */

            s->audio_channels = (audio_flags & 1) + 1;

            /* bit 1 of the flags: 0 = 8 bit, 1 = 16 bit */

            s->audio_bits = (((audio_flags >> 1) & 1) + 1) * 8;

            /* bit 2 indicates compressed audio in version 1 opcode */

            if ((opcode_version == 1) && (audio_flags & 0x4))

                s->audio_type = AV_CODEC_ID_INTERPLAY_DPCM;

            else if (s->audio_bits == 16)

                s->audio_type = AV_CODEC_ID_PCM_S16LE;

            else

                s->audio_type = AV_CODEC_ID_PCM_U8;

            av_log(s->avf, AV_LOG_TRACE, "audio: %d bits, %d Hz, %s, %s format\n",

                    s->audio_bits, s->audio_sample_rate,

                    (s->audio_channels == 2) ? "stereo" : "mono",

                    (s->audio_type == AV_CODEC_ID_INTERPLAY_DPCM) ?

                    "Interplay audio" : "PCM");

            break;



        case OPCODE_START_STOP_AUDIO:

            av_log(s->avf, AV_LOG_TRACE, "start/stop audio\n");

            avio_skip(pb, opcode_size);

            break;



        case OPCODE_INIT_VIDEO_BUFFERS:

            av_log(s->avf, AV_LOG_TRACE, "initialize video buffers\n");

            if ((opcode_version > 2) || (opcode_size > 8) || opcode_size < 4

                || opcode_version == 2 && opcode_size < 8

            ) {

                av_log(s->avf, AV_LOG_TRACE, "bad init_video_buffers opcode\n");

                chunk_type = CHUNK_BAD;

                break;

            }

            if (avio_read(pb, scratch, opcode_size) !=

                opcode_size) {

                chunk_type = CHUNK_BAD;

                break;

            }

            width  = AV_RL16(&scratch[0]) * 8;

            height = AV_RL16(&scratch[2]) * 8;

            if (width != s->video_width) {

                s->video_width = width;

                s->changed++;

            }

            if (height != s->video_height) {

                s->video_height = height;

                s->changed++;

            }

            if (opcode_version < 2 || !AV_RL16(&scratch[6])) {

                s->video_bpp = 8;

            } else {

                s->video_bpp = 16;

            }

            av_log(s->avf, AV_LOG_TRACE, "video resolution: %d x %d\n",

                    s->video_width, s->video_height);

            break;



        case OPCODE_UNKNOWN_06:

        case OPCODE_UNKNOWN_0E:

        case OPCODE_UNKNOWN_10:

        case OPCODE_UNKNOWN_12:

        case OPCODE_UNKNOWN_13:

        case OPCODE_UNKNOWN_14:

        case OPCODE_UNKNOWN_15:

            av_log(s->avf, AV_LOG_TRACE, "unknown (but documented) opcode %02X\n", opcode_type);

            avio_skip(pb, opcode_size);

            break;



        case OPCODE_SEND_BUFFER:

            av_log(s->avf, AV_LOG_TRACE, "send buffer\n");

            avio_skip(pb, opcode_size);

            s->send_buffer = 1;

            break;



        case OPCODE_AUDIO_FRAME:

            av_log(s->avf, AV_LOG_TRACE, "audio frame\n");



            /* log position and move on for now */

            s->audio_chunk_offset = avio_tell(pb);

            s->audio_chunk_size = opcode_size;

            avio_skip(pb, opcode_size);

            break;



        case OPCODE_SILENCE_FRAME:

            av_log(s->avf, AV_LOG_TRACE, "silence frame\n");

            avio_skip(pb, opcode_size);

            break;



        case OPCODE_INIT_VIDEO_MODE:

            av_log(s->avf, AV_LOG_TRACE, "initialize video mode\n");

            avio_skip(pb, opcode_size);

            break;



        case OPCODE_CREATE_GRADIENT:

            av_log(s->avf, AV_LOG_TRACE, "create gradient\n");

            avio_skip(pb, opcode_size);

            break;



        case OPCODE_SET_PALETTE:

            av_log(s->avf, AV_LOG_TRACE, "set palette\n");

            /* check for the logical maximum palette size

             * (3 * 256 + 4 bytes) */

            if (opcode_size > 0x304 || opcode_size < 4) {

                av_log(s->avf, AV_LOG_TRACE, "demux_ipmovie: set_palette opcode with invalid size\n");

                chunk_type = CHUNK_BAD;

                break;

            }

            if (avio_read(pb, scratch, opcode_size) != opcode_size) {

                chunk_type = CHUNK_BAD;

                break;

            }



            /* load the palette into internal data structure */

            first_color = AV_RL16(&scratch[0]);

            last_color = first_color + AV_RL16(&scratch[2]) - 1;

            /* sanity check (since they are 16 bit values) */

            if (   (first_color > 0xFF) || (last_color > 0xFF)

                || (last_color - first_color + 1)*3 + 4 > opcode_size) {

                av_log(s->avf, AV_LOG_TRACE, "demux_ipmovie: set_palette indexes out of range (%d -> %d)\n",

                    first_color, last_color);

                chunk_type = CHUNK_BAD;

                break;

            }

            j = 4;  /* offset of first palette data */

            for (i = first_color; i <= last_color; i++) {

                /* the palette is stored as a 6-bit VGA palette, thus each

                 * component is shifted up to a 8-bit range */

                r = scratch[j++] * 4;

                g = scratch[j++] * 4;

                b = scratch[j++] * 4;

                s->palette[i] = (0xFFU << 24) | (r << 16) | (g << 8) | (b);

                s->palette[i] |= s->palette[i] >> 6 & 0x30303;

            }

            s->has_palette = 1;

            break;



        case OPCODE_SET_PALETTE_COMPRESSED:

            av_log(s->avf, AV_LOG_TRACE, "set palette compressed\n");

            avio_skip(pb, opcode_size);

            break;



        case OPCODE_SET_DECODING_MAP:

            av_log(s->avf, AV_LOG_TRACE, "set decoding map\n");



            /* log position and move on for now */

            s->decode_map_chunk_offset = avio_tell(pb);

            s->decode_map_chunk_size = opcode_size;

            avio_skip(pb, opcode_size);

            break;



        case OPCODE_VIDEO_DATA_11:

            av_log(s->avf, AV_LOG_TRACE, "set video data\n");

            s->frame_format = 0x11;



            /* log position and move on for now */

            s->video_chunk_offset = avio_tell(pb);

            s->video_chunk_size = opcode_size;

            avio_skip(pb, opcode_size);

            break;



        default:

            av_log(s->avf, AV_LOG_TRACE, "*** unknown opcode type\n");

            chunk_type = CHUNK_BAD;

            break;



        }

    }



    if (s->avf->nb_streams == 1 && s->audio_type)

        init_audio(s->avf);



    /* make a note of where the stream is sitting */

    s->next_chunk_offset = avio_tell(pb);



    /* dispatch the first of any pending packets */

    if ((chunk_type == CHUNK_VIDEO) || (chunk_type == CHUNK_AUDIO_ONLY))

        chunk_type = load_ipmovie_packet(s, pb, pkt);



    return chunk_type;

}
