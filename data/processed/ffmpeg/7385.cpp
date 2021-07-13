static int ffm_read_data(AVFormatContext *s,

                         uint8_t *buf, int size, int header)

{

    FFMContext *ffm = s->priv_data;

    AVIOContext *pb = s->pb;

    int len, fill_size, size1, frame_offset;

    uint32_t id;

    int64_t last_pos = -1;



    size1 = size;

    while (size > 0) {

    redo:

        len = ffm->packet_end - ffm->packet_ptr;

        if (len < 0)

            return -1;

        if (len > size)

            len = size;

        if (len == 0) {

            if (avio_tell(pb) == ffm->file_size) {

                if (ffm->server_attached) {

                    avio_seek(pb, ffm->packet_size, SEEK_SET);

                } else

                    return AVERROR_EOF;

            }

    retry_read:

            if (pb->buffer_size != ffm->packet_size) {

                int64_t tell = avio_tell(pb);

                int ret = ffio_set_buf_size(pb, ffm->packet_size);

                if (ret < 0)

                    return ret;

                avio_seek(pb, tell, SEEK_SET);

            }

            id = avio_rb16(pb); /* PACKET_ID */

            if (id != PACKET_ID) {

                if (ffm_resync(s, id) < 0)

                    return -1;

                last_pos = avio_tell(pb);

            }

            fill_size = avio_rb16(pb);

            ffm->dts = avio_rb64(pb);

            frame_offset = avio_rb16(pb);

            avio_read(pb, ffm->packet, ffm->packet_size - FFM_HEADER_SIZE);

            if (ffm->packet_size < FFM_HEADER_SIZE + fill_size || frame_offset < 0) {

                return -1;

            }

            ffm->packet_end = ffm->packet + (ffm->packet_size - FFM_HEADER_SIZE - fill_size);

            /* if first packet or resynchronization packet, we must

               handle it specifically */

            if (ffm->first_packet || (frame_offset & 0x8000)) {

                if (!frame_offset) {

                    /* This packet has no frame headers in it */

                    if (avio_tell(pb) >= ffm->packet_size * 3LL) {

                        int64_t seekback = FFMIN(ffm->packet_size * 2LL, avio_tell(pb) - last_pos);

                        seekback = FFMAX(seekback, 0);

                        avio_seek(pb, -seekback, SEEK_CUR);

                        goto retry_read;

                    }

                    /* This is bad, we cannot find a valid frame header */

                    return 0;

                }

                ffm->first_packet = 0;

                if ((frame_offset & 0x7fff) < FFM_HEADER_SIZE) {

                    ffm->packet_end = ffm->packet_ptr;

                    return -1;

                }

                ffm->packet_ptr = ffm->packet + (frame_offset & 0x7fff) - FFM_HEADER_SIZE;

                if (!header)

                    break;

            } else {

                ffm->packet_ptr = ffm->packet;

            }

            goto redo;

        }

        memcpy(buf, ffm->packet_ptr, len);

        buf += len;

        ffm->packet_ptr += len;

        size -= len;

        header = 0;

    }

    return size1 - size;

}
