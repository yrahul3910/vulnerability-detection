static int opt_default(const char *opt, const char *arg){

    int type;

    const AVOption *o= NULL;

    int opt_types[]={AV_OPT_FLAG_VIDEO_PARAM, AV_OPT_FLAG_AUDIO_PARAM, 0, AV_OPT_FLAG_SUBTITLE_PARAM, 0};



    for(type=0; type<CODEC_TYPE_NB; type++){

        const AVOption *o2 = av_find_opt(avctx_opts[0], opt, NULL, opt_types[type], opt_types[type]);

        if(o2)

            o = av_set_string(avctx_opts[type], opt, arg);

    }

    if(!o)

        o = av_set_string(avformat_opts, opt, arg);

    if(!o)

        o = av_set_string(sws_opts, opt, arg);

    if(!o){

        if(opt[0] == 'a')

            o = av_set_string(avctx_opts[CODEC_TYPE_AUDIO], opt+1, arg);

        else if(opt[0] == 'v')

            o = av_set_string(avctx_opts[CODEC_TYPE_VIDEO], opt+1, arg);

        else if(opt[0] == 's')

            o = av_set_string(avctx_opts[CODEC_TYPE_SUBTITLE], opt+1, arg);

    }

    if(!o)

        return -1;



//    av_log(NULL, AV_LOG_ERROR, "%s:%s: %f 0x%0X\n", opt, arg, av_get_double(avctx_opts, opt, NULL), (int)av_get_int(avctx_opts, opt, NULL));



    //FIXME we should always use avctx_opts, ... for storing options so there wont be any need to keep track of whats set over this

    opt_names= av_realloc(opt_names, sizeof(void*)*(opt_name_count+1));

    opt_names[opt_name_count++]= o->name;



#ifdef CONFIG_FFM_MUXER

    /* disable generate of real time pts in ffm (need to be supressed anyway) */

    if(avctx_opts[0]->flags & CODEC_FLAG_BITEXACT)

        ffm_nopts = 1;

#endif



    if(avctx_opts[0]->debug)

        av_log_set_level(AV_LOG_DEBUG);

    return 0;

}
