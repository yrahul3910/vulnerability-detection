static void put_frame(

                    AVFormatContext *s,

                    ASFStream       *stream,

                    AVStream        *avst,

                    int             timestamp,

                    const uint8_t   *buf,

                    int             m_obj_size,

                    int             flags

                )

{

    ASFContext *asf = s->priv_data;

    int m_obj_offset, payload_len, frag_len1;



    m_obj_offset = 0;

    while (m_obj_offset < m_obj_size) {

        payload_len = m_obj_size - m_obj_offset;

        if (asf->packet_timestamp_start == -1) {

            asf->multi_payloads_present = (payload_len < MULTI_PAYLOAD_CONSTANT);



            asf->packet_size_left = PACKET_SIZE;

            if (asf->multi_payloads_present){

                frag_len1 = MULTI_PAYLOAD_CONSTANT - 1;

            }

            else {

                frag_len1 = SINGLE_PAYLOAD_DATA_LENGTH;

            }

            asf->packet_timestamp_start = timestamp;

        }

        else {

            // multi payloads

            frag_len1 = asf->packet_size_left - PAYLOAD_HEADER_SIZE_MULTIPLE_PAYLOADS - PACKET_HEADER_MIN_SIZE - 1;



            asf->packet_timestamp_start = timestamp;



            if(frag_len1 < payload_len && avst->codec->codec_type == CODEC_TYPE_AUDIO){

                flush_packet(s);

                continue;

            }

        }

        if (frag_len1 > 0) {

            if (payload_len > frag_len1)

                payload_len = frag_len1;

            else if (payload_len == (frag_len1 - 1))

                payload_len = frag_len1 - 2;  //additional byte need to put padding length



            put_payload_header(s, stream, timestamp+PREROLL_TIME, m_obj_size, m_obj_offset, payload_len, flags);

            put_buffer(&asf->pb, buf, payload_len);



            if (asf->multi_payloads_present)

                asf->packet_size_left -= (payload_len + PAYLOAD_HEADER_SIZE_MULTIPLE_PAYLOADS);

            else

                asf->packet_size_left -= (payload_len + PAYLOAD_HEADER_SIZE_SINGLE_PAYLOAD);

            asf->packet_timestamp_end = timestamp;



            asf->packet_nb_payloads++;

        } else {

            payload_len = 0;

        }

        m_obj_offset += payload_len;

        buf += payload_len;



        if (!asf->multi_payloads_present)

            flush_packet(s);

        else if (asf->packet_size_left <= (PAYLOAD_HEADER_SIZE_MULTIPLE_PAYLOADS + PACKET_HEADER_MIN_SIZE + 1))

            flush_packet(s);

    }

    stream->seq++;

}
