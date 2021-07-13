int av_get_packet(AVIOContext *s, AVPacket *pkt, int size)

{

    int ret= av_new_packet(pkt, size);



    if(ret<0)

        return ret;



    pkt->pos= avio_tell(s);



    ret= avio_read(s, pkt->data, size);

    if(ret<=0)

        av_free_packet(pkt);

    else

        av_shrink_packet(pkt, ret);



    return ret;

}
