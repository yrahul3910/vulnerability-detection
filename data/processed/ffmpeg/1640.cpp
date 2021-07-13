static int mov_write_sidx_tag(AVIOContext *pb,

                              MOVTrack *track, int ref_size, int total_sidx_size)

{

    int64_t pos = avio_tell(pb), offset_pos, end_pos;

    int64_t presentation_time, duration, offset;

    int starts_with_SAP, i, entries;



    if (track->entry) {

        entries = 1;

        presentation_time = track->start_dts + track->frag_start +

                            track->cluster[0].cts;

        duration = track->end_pts -

                   (track->cluster[0].dts + track->cluster[0].cts);

        starts_with_SAP = track->cluster[0].flags & MOV_SYNC_SAMPLE;



        // pts<0 should be cut away using edts

        if (presentation_time < 0) {

            duration += presentation_time;

            presentation_time = 0;

        }

    } else {

        entries = track->nb_frag_info;



        presentation_time = track->frag_info[0].time;

    }



    avio_wb32(pb, 0); /* size */

    ffio_wfourcc(pb, "sidx");

    avio_w8(pb, 1); /* version */

    avio_wb24(pb, 0);

    avio_wb32(pb, track->track_id); /* reference_ID */

    avio_wb32(pb, track->timescale); /* timescale */

    avio_wb64(pb, presentation_time); /* earliest_presentation_time */

    offset_pos = avio_tell(pb);

    avio_wb64(pb, 0); /* first_offset (offset to referenced moof) */

    avio_wb16(pb, 0); /* reserved */



    avio_wb16(pb, entries); /* reference_count */

    for (i = 0; i < entries; i++) {

        if (!track->entry) {

            if (i > 1 && track->frag_info[i].offset != track->frag_info[i - 1].offset + track->frag_info[i - 1].size) {

               av_log(NULL, AV_LOG_ERROR, "Non-consecutive fragments, writing incorrect sidx\n");

            }

            duration = track->frag_info[i].duration;

            ref_size = track->frag_info[i].size;

            starts_with_SAP = 1;

        }

        avio_wb32(pb, (0 << 31) | (ref_size & 0x7fffffff)); /* reference_type (0 = media) | referenced_size */

        avio_wb32(pb, duration); /* subsegment_duration */

        avio_wb32(pb, (starts_with_SAP << 31) | (0 << 28) | 0); /* starts_with_SAP | SAP_type | SAP_delta_time */

    }



    end_pos = avio_tell(pb);

    offset = pos + total_sidx_size - end_pos;

    avio_seek(pb, offset_pos, SEEK_SET);

    avio_wb64(pb, offset);

    avio_seek(pb, end_pos, SEEK_SET);

    return update_size(pb, pos);

}