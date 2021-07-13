static int fourxm_probe(AVProbeData *p)

{

    if (p->buf_size < 12)

        return 0;



    if ((AV_RL32(&p->buf[0]) != RIFF_TAG) ||

        (AV_RL32(&p->buf[8]) != _4XMV_TAG))

        return 0;



    return AVPROBE_SCORE_MAX;

}
