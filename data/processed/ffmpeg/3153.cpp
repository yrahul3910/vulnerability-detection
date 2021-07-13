static int mjpegb_decode_frame(AVCodecContext *avctx, 

                              void *data, int *data_size,

                              uint8_t *buf, int buf_size)

{

    MJpegDecodeContext *s = avctx->priv_data;

    uint8_t *buf_end, *buf_ptr;

    AVFrame *picture = data;

    GetBitContext hgb; /* for the header */

    uint32_t dqt_offs, dht_offs, sof_offs, sos_offs, second_field_offs;

    uint32_t field_size, sod_offs;



    /* no supplementary picture */

    if (buf_size == 0)

        return 0;



    buf_ptr = buf;

    buf_end = buf + buf_size;

    

read_header:

    /* reset on every SOI */

    s->restart_interval = 0;

    s->mjpb_skiptosod = 0;



    init_get_bits(&hgb, buf_ptr, /*buf_size*/(buf_end - buf_ptr)*8);



    skip_bits(&hgb, 32); /* reserved zeros */

    

    if (get_bits_long(&hgb, 32) != be2me_32(ff_get_fourcc("mjpg")))

    {

	dprintf("not mjpeg-b (bad fourcc)\n");

	return 0;

    }



    field_size = get_bits_long(&hgb, 32); /* field size */

    dprintf("field size: 0x%x\n", field_size);

    skip_bits(&hgb, 32); /* padded field size */

    second_field_offs = get_bits_long(&hgb, 32);

    dprintf("second field offs: 0x%x\n", second_field_offs);

    if (second_field_offs)

	s->interlaced = 1;



    dqt_offs = get_bits_long(&hgb, 32);

    dprintf("dqt offs: 0x%x\n", dqt_offs);

    if (dqt_offs)

    {

	init_get_bits(&s->gb, buf+dqt_offs, (buf_end - (buf+dqt_offs))*8);

	s->start_code = DQT;

	mjpeg_decode_dqt(s);

    }

    

    dht_offs = get_bits_long(&hgb, 32);

    dprintf("dht offs: 0x%x\n", dht_offs);

    if (dht_offs)

    {

	init_get_bits(&s->gb, buf+dht_offs, (buf_end - (buf+dht_offs))*8);

	s->start_code = DHT;

	mjpeg_decode_dht(s);

    }



    sof_offs = get_bits_long(&hgb, 32);

    dprintf("sof offs: 0x%x\n", sof_offs);

    if (sof_offs)

    {

	init_get_bits(&s->gb, buf+sof_offs, (buf_end - (buf+sof_offs))*8);

	s->start_code = SOF0;

	if (mjpeg_decode_sof(s) < 0)

	    return -1;

    }



    sos_offs = get_bits_long(&hgb, 32);

    dprintf("sos offs: 0x%x\n", sos_offs);

    sod_offs = get_bits_long(&hgb, 32);

    dprintf("sod offs: 0x%x\n", sod_offs);

    if (sos_offs)

    {

//	init_get_bits(&s->gb, buf+sos_offs, (buf_end - (buf+sos_offs))*8);

	init_get_bits(&s->gb, buf+sos_offs, field_size*8);

	s->mjpb_skiptosod = (sod_offs - sos_offs - show_bits(&s->gb, 16));

	s->start_code = SOS;

	mjpeg_decode_sos(s);

    }



    if (s->interlaced) {

        s->bottom_field ^= 1;

        /* if not bottom field, do not output image yet */

        if (s->bottom_field && second_field_offs)

	{

	    buf_ptr = buf + second_field_offs;

	    second_field_offs = 0;

	    goto read_header;

    	}

    }



    //XXX FIXME factorize, this looks very similar to the EOI code



    *picture= s->picture;

    *data_size = sizeof(AVFrame);

    

    if(!s->lossless){

        picture->quality= FFMAX(FFMAX(s->qscale[0], s->qscale[1]), s->qscale[2]); 

        picture->qstride= 0;

        picture->qscale_table= s->qscale_table;

        memset(picture->qscale_table, picture->quality, (s->width+15)/16);

        if(avctx->debug & FF_DEBUG_QP)

            av_log(avctx, AV_LOG_DEBUG, "QP: %d\n", picture->quality);

        picture->quality*= FF_QP2LAMBDA;

    }



    return buf_ptr - buf;

}
