static void estimate_timings_from_bit_rate(AVFormatContext *ic)

{

    int64_t filesize, duration;

    int i;

    AVStream *st;



    /* if bit_rate is already set, we believe it */

    if (ic->bit_rate <= 0) {

        int bit_rate = 0;

        for(i=0;i<ic->nb_streams;i++) {

            st = ic->streams[i];

            if (st->codec->bit_rate > 0) {

                if (INT_MAX - st->codec->bit_rate > bit_rate) {

                    bit_rate = 0;

                    break;

                }

                bit_rate += st->codec->bit_rate;

            }

        }

        ic->bit_rate = bit_rate;

    }



    /* if duration is already set, we believe it */

    if (ic->duration == AV_NOPTS_VALUE &&

        ic->bit_rate != 0) {

        filesize = ic->pb ? avio_size(ic->pb) : 0;

        if (filesize > 0) {

            for(i = 0; i < ic->nb_streams; i++) {

                st = ic->streams[i];

                duration= av_rescale(8*filesize, st->time_base.den, ic->bit_rate*(int64_t)st->time_base.num);

                if (st->duration == AV_NOPTS_VALUE)

                    st->duration = duration;

            }

        }

    }

}
