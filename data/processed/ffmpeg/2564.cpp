void ff_rtp_send_aac(AVFormatContext *s1, const uint8_t *buff, int size)

{

    RTPMuxContext *s = s1->priv_data;

    int len, max_packet_size;

    uint8_t *p;



    /* skip ADTS header, if present */

    if ((s1->streams[0]->codec->extradata_size) == 0) {

        size -= 7;

        buff += 7;

    }

    max_packet_size = s->max_payload_size - MAX_AU_HEADERS_SIZE;



    /* test if the packet must be sent */

    len = (s->buf_ptr - s->buf);

    if ((s->num_frames == MAX_FRAMES_PER_PACKET) || (len && (len + size) > s->max_payload_size)) {

        int au_size = s->num_frames * 2;



        p = s->buf + MAX_AU_HEADERS_SIZE - au_size - 2;

        if (p != s->buf) {

            memmove(p + 2, s->buf + 2, au_size);

        }

        /* Write the AU header size */

        p[0] = ((au_size * 8) & 0xFF) >> 8;

        p[1] = (au_size * 8) & 0xFF;



        ff_rtp_send_data(s1, p, s->buf_ptr - p, 1);



        s->num_frames = 0;

    }

    if (s->num_frames == 0) {

        s->buf_ptr = s->buf + MAX_AU_HEADERS_SIZE;

        s->timestamp = s->cur_timestamp;

    }



    if (size <= max_packet_size) {

        p = s->buf + s->num_frames++ * 2 + 2;

        *p++ = size >> 5;

        *p = (size & 0x1F) << 3;

        memcpy(s->buf_ptr, buff, size);

        s->buf_ptr += size;

    } else {

        if (s->buf_ptr != s->buf + MAX_AU_HEADERS_SIZE) {

            av_log(s1, AV_LOG_ERROR, "Strange...\n");

            av_abort();

        }

        max_packet_size = s->max_payload_size - 4;

        p = s->buf;

        p[0] = 0;

        p[1] = 16;

        while (size > 0) {

            len = FFMIN(size, max_packet_size);

            p[2] = len >> 5;

            p[3] = (size & 0x1F) << 3;

            memcpy(p + 4, buff, len);

            ff_rtp_send_data(s1, p, len + 4, len == size);

            size -= len;

            buff += len;

        }

    }

}
