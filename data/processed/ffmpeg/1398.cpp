static int read_packet(AVFormatContext *s1, AVPacket *pkt)

{

    VideoDemuxData *s = s1->priv_data;

    char filename_bytes[1024];

    char *filename = filename_bytes;

    int i;

    int size[3]={0}, ret[3]={0};

    AVIOContext *f[3] = {NULL};

    AVCodecContext *codec= s1->streams[0]->codec;



    if (!s->is_pipe) {

        /* loop over input */

        if (s->loop && s->img_number > s->img_last) {

            s->img_number = s->img_first;

        }

        if (s->img_number > s->img_last)

            return AVERROR_EOF;

        if (s->use_glob) {

#if HAVE_GLOB

            filename = s->globstate.gl_pathv[s->img_number];

#endif

        } else {

        if (av_get_frame_filename(filename_bytes, sizeof(filename_bytes),

                                  s->path, s->img_number)<0 && s->img_number > 1)

            return AVERROR(EIO);

        }

        for(i=0; i<3; i++){

            if (avio_open2(&f[i], filename, AVIO_FLAG_READ,

                           &s1->interrupt_callback, NULL) < 0) {

                if(i>=1)

                    break;

                av_log(s1, AV_LOG_ERROR, "Could not open file : %s\n",filename);

                return AVERROR(EIO);

            }

            size[i]= avio_size(f[i]);



            if(!s->split_planes)

                break;

            filename[ strlen(filename) - 1 ]= 'U' + i;

        }



        if(codec->codec_id == AV_CODEC_ID_RAWVIDEO && !codec->width)

            infer_size(&codec->width, &codec->height, size[0]);

    } else {

        f[0] = s1->pb;

        if (url_feof(f[0]))

            return AVERROR(EIO);

        size[0]= 4096;

    }



    av_new_packet(pkt, size[0] + size[1] + size[2]);

    pkt->stream_index = 0;

    pkt->flags |= AV_PKT_FLAG_KEY;



    pkt->size= 0;

    for(i=0; i<3; i++){

        if(f[i]){

            ret[i]= avio_read(f[i], pkt->data + pkt->size, size[i]);

            if (!s->is_pipe)

                avio_close(f[i]);

            if(ret[i]>0)

                pkt->size += ret[i];

        }

    }



    if (ret[0] <= 0 || ret[1]<0 || ret[2]<0) {

        av_free_packet(pkt);

        return AVERROR(EIO); /* signal EOF */

    } else {

        s->img_count++;

        s->img_number++;

        return 0;

    }

}
