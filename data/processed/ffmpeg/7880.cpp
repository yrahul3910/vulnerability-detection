av_cold int MPV_common_init(MpegEncContext *s)

{

    int y_size, c_size, yc_size, i, mb_array_size, mv_table_size, x, y, threads;



    if(s->codec_id == CODEC_ID_MPEG2VIDEO && !s->progressive_sequence)

        s->mb_height = (s->height + 31) / 32 * 2;

    else

        s->mb_height = (s->height + 15) / 16;



    if(s->avctx->pix_fmt == PIX_FMT_NONE){

        av_log(s->avctx, AV_LOG_ERROR, "decoding to PIX_FMT_NONE is not supported.\n");

        return -1;

    }



    if(s->avctx->thread_count > MAX_THREADS || (s->avctx->thread_count > s->mb_height && s->mb_height)){

        av_log(s->avctx, AV_LOG_ERROR, "too many threads\n");

        return -1;

    }



    if((s->width || s->height) && avcodec_check_dimensions(s->avctx, s->width, s->height))

        return -1;



    dsputil_init(&s->dsp, s->avctx);

    ff_dct_common_init(s);



    s->flags= s->avctx->flags;

    s->flags2= s->avctx->flags2;



    s->mb_width  = (s->width  + 15) / 16;

    s->mb_stride = s->mb_width + 1;

    s->b8_stride = s->mb_width*2 + 1;

    s->b4_stride = s->mb_width*4 + 1;

    mb_array_size= s->mb_height * s->mb_stride;

    mv_table_size= (s->mb_height+2) * s->mb_stride + 1;



    /* set chroma shifts */

    avcodec_get_chroma_sub_sample(s->avctx->pix_fmt,&(s->chroma_x_shift),

                                                    &(s->chroma_y_shift) );



    /* set default edge pos, will be overriden in decode_header if needed */

    s->h_edge_pos= s->mb_width*16;

    s->v_edge_pos= s->mb_height*16;



    s->mb_num = s->mb_width * s->mb_height;



    s->block_wrap[0]=

    s->block_wrap[1]=

    s->block_wrap[2]=

    s->block_wrap[3]= s->b8_stride;

    s->block_wrap[4]=

    s->block_wrap[5]= s->mb_stride;



    y_size = s->b8_stride * (2 * s->mb_height + 1);

    c_size = s->mb_stride * (s->mb_height + 1);

    yc_size = y_size + 2 * c_size;



    /* convert fourcc to upper case */

    s->codec_tag = ff_toupper4(s->avctx->codec_tag);



    s->stream_codec_tag = ff_toupper4(s->avctx->stream_codec_tag);



    s->avctx->coded_frame= (AVFrame*)&s->current_picture;



    FF_ALLOCZ_OR_GOTO(s->avctx, s->mb_index2xy, (s->mb_num+1)*sizeof(int), fail) //error ressilience code looks cleaner with this

    for(y=0; y<s->mb_height; y++){

        for(x=0; x<s->mb_width; x++){

            s->mb_index2xy[ x + y*s->mb_width ] = x + y*s->mb_stride;

        }

    }

    s->mb_index2xy[ s->mb_height*s->mb_width ] = (s->mb_height-1)*s->mb_stride + s->mb_width; //FIXME really needed?



    if (s->encoding) {

        /* Allocate MV tables */

        FF_ALLOCZ_OR_GOTO(s->avctx, s->p_mv_table_base            , mv_table_size * 2 * sizeof(int16_t), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_forw_mv_table_base       , mv_table_size * 2 * sizeof(int16_t), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_back_mv_table_base       , mv_table_size * 2 * sizeof(int16_t), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_bidir_forw_mv_table_base , mv_table_size * 2 * sizeof(int16_t), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_bidir_back_mv_table_base , mv_table_size * 2 * sizeof(int16_t), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->b_direct_mv_table_base     , mv_table_size * 2 * sizeof(int16_t), fail)

        s->p_mv_table           = s->p_mv_table_base            + s->mb_stride + 1;

        s->b_forw_mv_table      = s->b_forw_mv_table_base       + s->mb_stride + 1;

        s->b_back_mv_table      = s->b_back_mv_table_base       + s->mb_stride + 1;

        s->b_bidir_forw_mv_table= s->b_bidir_forw_mv_table_base + s->mb_stride + 1;

        s->b_bidir_back_mv_table= s->b_bidir_back_mv_table_base + s->mb_stride + 1;

        s->b_direct_mv_table    = s->b_direct_mv_table_base     + s->mb_stride + 1;



        if(s->msmpeg4_version){

            FF_ALLOCZ_OR_GOTO(s->avctx, s->ac_stats, 2*2*(MAX_LEVEL+1)*(MAX_RUN+1)*2*sizeof(int), fail);

        }

        FF_ALLOCZ_OR_GOTO(s->avctx, s->avctx->stats_out, 256, fail);



        /* Allocate MB type table */

        FF_ALLOCZ_OR_GOTO(s->avctx, s->mb_type  , mb_array_size * sizeof(uint16_t), fail) //needed for encoding



        FF_ALLOCZ_OR_GOTO(s->avctx, s->lambda_table, mb_array_size * sizeof(int), fail)



        FF_ALLOCZ_OR_GOTO(s->avctx, s->q_intra_matrix  , 64*32   * sizeof(int), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->q_inter_matrix  , 64*32   * sizeof(int), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->q_intra_matrix16, 64*32*2 * sizeof(uint16_t), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->q_inter_matrix16, 64*32*2 * sizeof(uint16_t), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->input_picture, MAX_PICTURE_COUNT * sizeof(Picture*), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->reordered_input_picture, MAX_PICTURE_COUNT * sizeof(Picture*), fail)



        if(s->avctx->noise_reduction){

            FF_ALLOCZ_OR_GOTO(s->avctx, s->dct_offset, 2 * 64 * sizeof(uint16_t), fail)

        }

    }

    FF_ALLOCZ_OR_GOTO(s->avctx, s->picture, MAX_PICTURE_COUNT * sizeof(Picture), fail)

    for(i = 0; i < MAX_PICTURE_COUNT; i++) {

        avcodec_get_frame_defaults((AVFrame *)&s->picture[i]);

    }



    FF_ALLOCZ_OR_GOTO(s->avctx, s->error_status_table, mb_array_size*sizeof(uint8_t), fail)



    if(s->codec_id==CODEC_ID_MPEG4 || (s->flags & CODEC_FLAG_INTERLACED_ME)){

        /* interlaced direct mode decoding tables */

            for(i=0; i<2; i++){

                int j, k;

                for(j=0; j<2; j++){

                    for(k=0; k<2; k++){

                        FF_ALLOCZ_OR_GOTO(s->avctx,    s->b_field_mv_table_base[i][j][k], mv_table_size * 2 * sizeof(int16_t), fail)

                        s->b_field_mv_table[i][j][k] = s->b_field_mv_table_base[i][j][k] + s->mb_stride + 1;

                    }

                    FF_ALLOCZ_OR_GOTO(s->avctx, s->b_field_select_table [i][j], mb_array_size * 2 * sizeof(uint8_t), fail)

                    FF_ALLOCZ_OR_GOTO(s->avctx, s->p_field_mv_table_base[i][j], mv_table_size * 2 * sizeof(int16_t), fail)

                    s->p_field_mv_table[i][j] = s->p_field_mv_table_base[i][j]+ s->mb_stride + 1;

                }

                FF_ALLOCZ_OR_GOTO(s->avctx, s->p_field_select_table[i], mb_array_size * 2 * sizeof(uint8_t), fail)

            }

    }

    if (s->out_format == FMT_H263) {

        /* ac values */

        FF_ALLOCZ_OR_GOTO(s->avctx, s->ac_val_base, yc_size * sizeof(int16_t) * 16, fail);

        s->ac_val[0] = s->ac_val_base + s->b8_stride + 1;

        s->ac_val[1] = s->ac_val_base + y_size + s->mb_stride + 1;

        s->ac_val[2] = s->ac_val[1] + c_size;



        /* cbp values */

        FF_ALLOCZ_OR_GOTO(s->avctx, s->coded_block_base, y_size, fail);

        s->coded_block= s->coded_block_base + s->b8_stride + 1;



        /* cbp, ac_pred, pred_dir */

        FF_ALLOCZ_OR_GOTO(s->avctx, s->cbp_table     , mb_array_size * sizeof(uint8_t), fail)

        FF_ALLOCZ_OR_GOTO(s->avctx, s->pred_dir_table, mb_array_size * sizeof(uint8_t), fail)

    }



    if (s->h263_pred || s->h263_plus || !s->encoding) {

        /* dc values */

        //MN: we need these for error resilience of intra-frames

        FF_ALLOCZ_OR_GOTO(s->avctx, s->dc_val_base, yc_size * sizeof(int16_t), fail);

        s->dc_val[0] = s->dc_val_base + s->b8_stride + 1;

        s->dc_val[1] = s->dc_val_base + y_size + s->mb_stride + 1;

        s->dc_val[2] = s->dc_val[1] + c_size;

        for(i=0;i<yc_size;i++)

            s->dc_val_base[i] = 1024;

    }



    /* which mb is a intra block */

    FF_ALLOCZ_OR_GOTO(s->avctx, s->mbintra_table, mb_array_size, fail);

    memset(s->mbintra_table, 1, mb_array_size);



    /* init macroblock skip table */

    FF_ALLOCZ_OR_GOTO(s->avctx, s->mbskip_table, mb_array_size+2, fail);

    //Note the +1 is for a quicker mpeg4 slice_end detection

    FF_ALLOCZ_OR_GOTO(s->avctx, s->prev_pict_types, PREV_PICT_TYPES_BUFFER_SIZE, fail);



    s->parse_context.state= -1;

    if((s->avctx->debug&(FF_DEBUG_VIS_QP|FF_DEBUG_VIS_MB_TYPE)) || (s->avctx->debug_mv)){

       s->visualization_buffer[0] = av_malloc((s->mb_width*16 + 2*EDGE_WIDTH) * s->mb_height*16 + 2*EDGE_WIDTH);

       s->visualization_buffer[1] = av_malloc((s->mb_width*16 + 2*EDGE_WIDTH) * s->mb_height*16 + 2*EDGE_WIDTH);

       s->visualization_buffer[2] = av_malloc((s->mb_width*16 + 2*EDGE_WIDTH) * s->mb_height*16 + 2*EDGE_WIDTH);

    }



    s->context_initialized = 1;



    s->thread_context[0]= s;

    threads = s->avctx->thread_count;



    for(i=1; i<threads; i++){

        s->thread_context[i]= av_malloc(sizeof(MpegEncContext));

        memcpy(s->thread_context[i], s, sizeof(MpegEncContext));

    }



    for(i=0; i<threads; i++){

        if(init_duplicate_context(s->thread_context[i], s) < 0)

           goto fail;

        s->thread_context[i]->start_mb_y= (s->mb_height*(i  ) + s->avctx->thread_count/2) / s->avctx->thread_count;

        s->thread_context[i]->end_mb_y  = (s->mb_height*(i+1) + s->avctx->thread_count/2) / s->avctx->thread_count;

    }



    return 0;

 fail:

    MPV_common_end(s);

    return -1;

}
