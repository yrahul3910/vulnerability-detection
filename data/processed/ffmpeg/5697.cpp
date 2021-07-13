static int opus_packet(AVFormatContext *avf, int idx)

{

    struct ogg *ogg              = avf->priv_data;

    struct ogg_stream *os        = &ogg->streams[idx];

    AVStream *st                 = avf->streams[idx];

    struct oggopus_private *priv = os->private;

    uint8_t *packet              = os->buf + os->pstart;



    if (!os->psize)

        return AVERROR_INVALIDDATA;



    if ((!os->lastpts || os->lastpts == AV_NOPTS_VALUE) && !(os->flags & OGG_FLAG_EOS)) {

        int seg, d;

        int duration;

        uint8_t *last_pkt  = os->buf + os->pstart;

        uint8_t *next_pkt  = last_pkt;



        duration = 0;

        seg = os->segp;

        d = opus_duration(last_pkt, os->psize);

        if (d < 0) {

            os->pflags |= AV_PKT_FLAG_CORRUPT;

            return 0;

        }

        duration += d;

        last_pkt = next_pkt =  next_pkt + os->psize;

        for (; seg < os->nsegs; seg++) {

            if (os->segments[seg] < 255) {

                int d = opus_duration(last_pkt, os->segments[seg]);

                if (d < 0) {

                    duration = os->granule;

                    break;

                }

                duration += d;

                last_pkt  = next_pkt + os->segments[seg];

            }

            next_pkt += os->segments[seg];

        }

        os->lastpts                 =

        os->lastdts                 = os->granule - duration;

    }



    os->pduration = opus_duration(packet, os->psize);

    if (os->lastpts != AV_NOPTS_VALUE) {

        if (st->start_time == AV_NOPTS_VALUE)

            st->start_time = os->lastpts;

        priv->cur_dts = os->lastdts = os->lastpts -= priv->pre_skip;

    }



    priv->cur_dts += os->pduration;

    if ((os->flags & OGG_FLAG_EOS)) {

        int64_t skip = priv->cur_dts - os->granule + priv->pre_skip;

        skip = FFMIN(skip, os->pduration);

        if (skip > 0) {

            os->pduration = skip < os->pduration ? os->pduration - skip : 1;

            os->end_trimming = skip;

            av_log(avf, AV_LOG_DEBUG,

                   "Last packet was truncated to %d due to end trimming.\n",

                   os->pduration);

        }

    }



    return 0;

}
