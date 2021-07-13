static int read_pakt_chunk(AVFormatContext *s, int64_t size)

{

    AVIOContext *pb = s->pb;

    AVStream *st      = s->streams[0];

    CaffContext *caf  = s->priv_data;

    int64_t pos = 0, ccount;

    int num_packets, i;



    ccount = avio_tell(pb);



    num_packets = avio_rb64(pb);

    if (num_packets < 0 || INT32_MAX / sizeof(AVIndexEntry) < num_packets)

        return AVERROR_INVALIDDATA;



    st->nb_frames  = avio_rb64(pb); /* valid frames */

    st->nb_frames += avio_rb32(pb); /* priming frames */

    st->nb_frames += avio_rb32(pb); /* remainder frames */



    st->duration = 0;

    for (i = 0; i < num_packets; i++) {

        av_add_index_entry(s->streams[0], pos, st->duration, 0, 0, AVINDEX_KEYFRAME);

        pos += caf->bytes_per_packet ? caf->bytes_per_packet : ff_mp4_read_descr_len(pb);

        st->duration += caf->frames_per_packet ? caf->frames_per_packet : ff_mp4_read_descr_len(pb);

    }



    if (avio_tell(pb) - ccount != size) {

        av_log(s, AV_LOG_ERROR, "error reading packet table\n");

        return -1;

    }



    caf->num_bytes = pos;

    return 0;

}
