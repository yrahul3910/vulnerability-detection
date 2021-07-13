static int avi_read_seek(AVFormatContext *s, int stream_index, int64_t timestamp, int flags)

{

    AVIContext *avi = s->priv_data;

    AVStream *st;

    int i, index;

    int64_t pos;

    AVIStream *ast;



    if (!avi->index_loaded) {

        /* we only load the index on demand */

        avi_load_index(s);

        avi->index_loaded = 1;

    }

    assert(stream_index>= 0);



    st = s->streams[stream_index];

    ast= st->priv_data;

    index= av_index_search_timestamp(st, timestamp * FFMAX(ast->sample_size, 1), flags);

    if(index<0)

        return -1;



    /* find the position */

    pos = st->index_entries[index].pos;

    timestamp = st->index_entries[index].timestamp / FFMAX(ast->sample_size, 1);



    av_dlog(s, "XX %"PRId64" %d %"PRId64"\n",

            timestamp, index, st->index_entries[index].timestamp);



    if (CONFIG_DV_DEMUXER && avi->dv_demux) {

        /* One and only one real stream for DV in AVI, and it has video  */

        /* offsets. Calling with other stream indexes should have failed */

        /* the av_index_search_timestamp call above.                     */

        assert(stream_index == 0);



        /* Feed the DV video stream version of the timestamp to the */

        /* DV demux so it can synthesize correct timestamps.        */

        ff_dv_offset_reset(avi->dv_demux, timestamp);



        avio_seek(s->pb, pos, SEEK_SET);

        avi->stream_index= -1;

        return 0;

    }



    for(i = 0; i < s->nb_streams; i++) {

        AVStream *st2 = s->streams[i];

        AVIStream *ast2 = st2->priv_data;



        ast2->packet_size=

        ast2->remaining= 0;



        if (ast2->sub_ctx) {

            seek_subtitle(st, st2, timestamp);

            continue;

        }



        if (st2->nb_index_entries <= 0)

            continue;



//        assert(st2->codec->block_align);

        assert((int64_t)st2->time_base.num*ast2->rate == (int64_t)st2->time_base.den*ast2->scale);

        index = av_index_search_timestamp(

                st2,

                av_rescale_q(timestamp, st->time_base, st2->time_base) * FFMAX(ast2->sample_size, 1),

                flags | AVSEEK_FLAG_BACKWARD);

        if(index<0)

            index=0;



        if(!avi->non_interleaved){

            while(index>0 && st2->index_entries[index].pos > pos)

                index--;

            while(index+1 < st2->nb_index_entries && st2->index_entries[index].pos < pos)

                index++;

        }



        av_dlog(s, "%"PRId64" %d %"PRId64"\n",

                timestamp, index, st2->index_entries[index].timestamp);

        /* extract the current frame number */

        ast2->frame_offset = st2->index_entries[index].timestamp;

    }



    /* do the seek */

    avio_seek(s->pb, pos, SEEK_SET);

    avi->stream_index= -1;

    return 0;

}
