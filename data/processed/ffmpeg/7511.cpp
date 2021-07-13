void ff_er_frame_end(MpegEncContext *s){

    int i, mb_x, mb_y, error, error_type, dc_error, mv_error, ac_error;

    int distance;

    int threshold_part[4]= {100,100,100};

    int threshold= 50;

    int is_intra_likely;

    int size = s->b8_stride * 2 * s->mb_height;

    Picture *pic= s->current_picture_ptr;



    if(!s->error_recognition || s->error_count==0 || s->avctx->lowres ||

       s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU ||

       s->error_count==3*s->mb_width*(s->avctx->skip_top + s->avctx->skip_bottom)) return;



    if(s->current_picture.motion_val[0] == NULL){

        av_log(s->avctx, AV_LOG_ERROR, "Warning MVs not available\n");



        for(i=0; i<2; i++){

            pic->ref_index[i]= av_mallocz(size * sizeof(uint8_t));

            pic->motion_val_base[i]= av_mallocz((size+4) * 2 * sizeof(uint16_t));

            pic->motion_val[i]= pic->motion_val_base[i]+4;

        }

        pic->motion_subsample_log2= 3;

        s->current_picture= *s->current_picture_ptr;

    }



    for(i=0; i<2; i++){

        if(pic->ref_index[i])

            memset(pic->ref_index[i], 0, size * sizeof(uint8_t));

    }



    if(s->avctx->debug&FF_DEBUG_ER){

        for(mb_y=0; mb_y<s->mb_height; mb_y++){

            for(mb_x=0; mb_x<s->mb_width; mb_x++){

                int status= s->error_status_table[mb_x + mb_y*s->mb_stride];



                av_log(s->avctx, AV_LOG_DEBUG, "%2X ", status);

            }

            av_log(s->avctx, AV_LOG_DEBUG, "\n");

        }

    }



#if 1

    /* handle overlapping slices */

    for(error_type=1; error_type<=3; error_type++){

        int end_ok=0;



        for(i=s->mb_num-1; i>=0; i--){

            const int mb_xy= s->mb_index2xy[i];

            int error= s->error_status_table[mb_xy];



            if(error&(1<<error_type))

                end_ok=1;

            if(error&(8<<error_type))

                end_ok=1;



            if(!end_ok)

                s->error_status_table[mb_xy]|= 1<<error_type;



            if(error&VP_START)

                end_ok=0;

        }

    }

#endif

#if 1

    /* handle slices with partitions of different length */

    if(s->partitioned_frame){

        int end_ok=0;



        for(i=s->mb_num-1; i>=0; i--){

            const int mb_xy= s->mb_index2xy[i];

            int error= s->error_status_table[mb_xy];



            if(error&AC_END)

                end_ok=0;

            if((error&MV_END) || (error&DC_END) || (error&AC_ERROR))

                end_ok=1;



            if(!end_ok)

                s->error_status_table[mb_xy]|= AC_ERROR;



            if(error&VP_START)

                end_ok=0;

        }

    }

#endif

    /* handle missing slices */

    if(s->error_recognition>=4){

        int end_ok=1;



        for(i=s->mb_num-2; i>=s->mb_width+100; i--){ //FIXME +100 hack

            const int mb_xy= s->mb_index2xy[i];

            int error1= s->error_status_table[mb_xy  ];

            int error2= s->error_status_table[s->mb_index2xy[i+1]];



            if(error1&VP_START)

                end_ok=1;



            if(   error2==(VP_START|DC_ERROR|AC_ERROR|MV_ERROR|AC_END|DC_END|MV_END)

               && error1!=(VP_START|DC_ERROR|AC_ERROR|MV_ERROR|AC_END|DC_END|MV_END)

               && ((error1&AC_END) || (error1&DC_END) || (error1&MV_END))){ //end & uninit

                end_ok=0;

            }



            if(!end_ok)

                s->error_status_table[mb_xy]|= DC_ERROR|AC_ERROR|MV_ERROR;

        }

    }



#if 1

    /* backward mark errors */

    distance=9999999;

    for(error_type=1; error_type<=3; error_type++){

        for(i=s->mb_num-1; i>=0; i--){

            const int mb_xy= s->mb_index2xy[i];

            int error= s->error_status_table[mb_xy];



            if(!s->mbskip_table[mb_xy]) //FIXME partition specific

                distance++;

            if(error&(1<<error_type))

                distance= 0;



            if(s->partitioned_frame){

                if(distance < threshold_part[error_type-1])

                    s->error_status_table[mb_xy]|= 1<<error_type;

            }else{

                if(distance < threshold)

                    s->error_status_table[mb_xy]|= 1<<error_type;

            }



            if(error&VP_START)

                distance= 9999999;

        }

    }

#endif



    /* forward mark errors */

    error=0;

    for(i=0; i<s->mb_num; i++){

        const int mb_xy= s->mb_index2xy[i];

        int old_error= s->error_status_table[mb_xy];



        if(old_error&VP_START)

            error= old_error& (DC_ERROR|AC_ERROR|MV_ERROR);

        else{

            error|= old_error& (DC_ERROR|AC_ERROR|MV_ERROR);

            s->error_status_table[mb_xy]|= error;

        }

    }

#if 1

    /* handle not partitioned case */

    if(!s->partitioned_frame){

        for(i=0; i<s->mb_num; i++){

            const int mb_xy= s->mb_index2xy[i];

            error= s->error_status_table[mb_xy];

            if(error&(AC_ERROR|DC_ERROR|MV_ERROR))

                error|= AC_ERROR|DC_ERROR|MV_ERROR;

            s->error_status_table[mb_xy]= error;

        }

    }

#endif



    dc_error= ac_error= mv_error=0;

    for(i=0; i<s->mb_num; i++){

        const int mb_xy= s->mb_index2xy[i];

        error= s->error_status_table[mb_xy];

        if(error&DC_ERROR) dc_error ++;

        if(error&AC_ERROR) ac_error ++;

        if(error&MV_ERROR) mv_error ++;

    }

    av_log(s->avctx, AV_LOG_INFO, "concealing %d DC, %d AC, %d MV errors\n", dc_error, ac_error, mv_error);



    is_intra_likely= is_intra_more_likely(s);



    /* set unknown mb-type to most likely */

    for(i=0; i<s->mb_num; i++){

        const int mb_xy= s->mb_index2xy[i];

        error= s->error_status_table[mb_xy];

        if(!((error&DC_ERROR) && (error&MV_ERROR)))

            continue;



        if(is_intra_likely)

            s->current_picture.mb_type[mb_xy]= MB_TYPE_INTRA4x4;

        else

            s->current_picture.mb_type[mb_xy]= MB_TYPE_16x16 | MB_TYPE_L0;

    }



    /* handle inter blocks with damaged AC */

    for(mb_y=0; mb_y<s->mb_height; mb_y++){

        for(mb_x=0; mb_x<s->mb_width; mb_x++){

            const int mb_xy= mb_x + mb_y * s->mb_stride;

            const int mb_type= s->current_picture.mb_type[mb_xy];

            error= s->error_status_table[mb_xy];



            if(IS_INTRA(mb_type)) continue; //intra

            if(error&MV_ERROR) continue;              //inter with damaged MV

            if(!(error&AC_ERROR)) continue;           //undamaged inter



            s->mv_dir = MV_DIR_FORWARD;

            s->mb_intra=0;

            s->mb_skipped=0;

            if(IS_8X8(mb_type)){

                int mb_index= mb_x*2 + mb_y*2*s->b8_stride;

                int j;

                s->mv_type = MV_TYPE_8X8;

                for(j=0; j<4; j++){

                    s->mv[0][j][0] = s->current_picture.motion_val[0][ mb_index + (j&1) + (j>>1)*s->b8_stride ][0];

                    s->mv[0][j][1] = s->current_picture.motion_val[0][ mb_index + (j&1) + (j>>1)*s->b8_stride ][1];

                }

            }else{

                s->mv_type = MV_TYPE_16X16;

                s->mv[0][0][0] = s->current_picture.motion_val[0][ mb_x*2 + mb_y*2*s->b8_stride ][0];

                s->mv[0][0][1] = s->current_picture.motion_val[0][ mb_x*2 + mb_y*2*s->b8_stride ][1];

            }



            s->dsp.clear_blocks(s->block[0]);



            s->mb_x= mb_x;

            s->mb_y= mb_y;

            decode_mb(s);

        }

    }



    /* guess MVs */

    if(s->pict_type==FF_B_TYPE){

        for(mb_y=0; mb_y<s->mb_height; mb_y++){

            for(mb_x=0; mb_x<s->mb_width; mb_x++){

                int xy= mb_x*2 + mb_y*2*s->b8_stride;

                const int mb_xy= mb_x + mb_y * s->mb_stride;

                const int mb_type= s->current_picture.mb_type[mb_xy];

                error= s->error_status_table[mb_xy];



                if(IS_INTRA(mb_type)) continue;

                if(!(error&MV_ERROR)) continue;           //inter with undamaged MV

                if(!(error&AC_ERROR)) continue;           //undamaged inter



                s->mv_dir = MV_DIR_FORWARD|MV_DIR_BACKWARD;

                s->mb_intra=0;

                s->mv_type = MV_TYPE_16X16;

                s->mb_skipped=0;



                if(s->pp_time){

                    int time_pp= s->pp_time;

                    int time_pb= s->pb_time;



                    s->mv[0][0][0] = s->next_picture.motion_val[0][xy][0]*time_pb/time_pp;

                    s->mv[0][0][1] = s->next_picture.motion_val[0][xy][1]*time_pb/time_pp;

                    s->mv[1][0][0] = s->next_picture.motion_val[0][xy][0]*(time_pb - time_pp)/time_pp;

                    s->mv[1][0][1] = s->next_picture.motion_val[0][xy][1]*(time_pb - time_pp)/time_pp;

                }else{

                    s->mv[0][0][0]= 0;

                    s->mv[0][0][1]= 0;

                    s->mv[1][0][0]= 0;

                    s->mv[1][0][1]= 0;

                }



                s->dsp.clear_blocks(s->block[0]);

                s->mb_x= mb_x;

                s->mb_y= mb_y;

                decode_mb(s);

            }

        }

    }else

        guess_mv(s);



#if CONFIG_MPEG_XVMC_DECODER

    /* the filters below are not XvMC compatible, skip them */

    if(s->avctx->xvmc_acceleration) goto ec_clean;

#endif

    /* fill DC for inter blocks */

    for(mb_y=0; mb_y<s->mb_height; mb_y++){

        for(mb_x=0; mb_x<s->mb_width; mb_x++){

            int dc, dcu, dcv, y, n;

            int16_t *dc_ptr;

            uint8_t *dest_y, *dest_cb, *dest_cr;

            const int mb_xy= mb_x + mb_y * s->mb_stride;

            const int mb_type= s->current_picture.mb_type[mb_xy];



            error= s->error_status_table[mb_xy];



            if(IS_INTRA(mb_type) && s->partitioned_frame) continue;

//            if(error&MV_ERROR) continue; //inter data damaged FIXME is this good?



            dest_y = s->current_picture.data[0] + mb_x*16 + mb_y*16*s->linesize;

            dest_cb= s->current_picture.data[1] + mb_x*8  + mb_y*8 *s->uvlinesize;

            dest_cr= s->current_picture.data[2] + mb_x*8  + mb_y*8 *s->uvlinesize;



            dc_ptr= &s->dc_val[0][mb_x*2 + mb_y*2*s->b8_stride];

            for(n=0; n<4; n++){

                dc=0;

                for(y=0; y<8; y++){

                    int x;

                    for(x=0; x<8; x++){

                       dc+= dest_y[x + (n&1)*8 + (y + (n>>1)*8)*s->linesize];

                    }

                }

                dc_ptr[(n&1) + (n>>1)*s->b8_stride]= (dc+4)>>3;

            }



            dcu=dcv=0;

            for(y=0; y<8; y++){

                int x;

                for(x=0; x<8; x++){

                    dcu+=dest_cb[x + y*(s->uvlinesize)];

                    dcv+=dest_cr[x + y*(s->uvlinesize)];

                }

            }

            s->dc_val[1][mb_x + mb_y*s->mb_stride]= (dcu+4)>>3;

            s->dc_val[2][mb_x + mb_y*s->mb_stride]= (dcv+4)>>3;

        }

    }

#if 1

    /* guess DC for damaged blocks */

    guess_dc(s, s->dc_val[0], s->mb_width*2, s->mb_height*2, s->b8_stride, 1);

    guess_dc(s, s->dc_val[1], s->mb_width  , s->mb_height  , s->mb_stride, 0);

    guess_dc(s, s->dc_val[2], s->mb_width  , s->mb_height  , s->mb_stride, 0);

#endif

    /* filter luma DC */

    filter181(s->dc_val[0], s->mb_width*2, s->mb_height*2, s->b8_stride);



#if 1

    /* render DC only intra */

    for(mb_y=0; mb_y<s->mb_height; mb_y++){

        for(mb_x=0; mb_x<s->mb_width; mb_x++){

            uint8_t *dest_y, *dest_cb, *dest_cr;

            const int mb_xy= mb_x + mb_y * s->mb_stride;

            const int mb_type= s->current_picture.mb_type[mb_xy];



            error= s->error_status_table[mb_xy];



            if(IS_INTER(mb_type)) continue;

            if(!(error&AC_ERROR)) continue;              //undamaged



            dest_y = s->current_picture.data[0] + mb_x*16 + mb_y*16*s->linesize;

            dest_cb= s->current_picture.data[1] + mb_x*8  + mb_y*8 *s->uvlinesize;

            dest_cr= s->current_picture.data[2] + mb_x*8  + mb_y*8 *s->uvlinesize;



            put_dc(s, dest_y, dest_cb, dest_cr, mb_x, mb_y);

        }

    }

#endif



    if(s->avctx->error_concealment&FF_EC_DEBLOCK){

        /* filter horizontal block boundaries */

        h_block_filter(s, s->current_picture.data[0], s->mb_width*2, s->mb_height*2, s->linesize  , 1);

        h_block_filter(s, s->current_picture.data[1], s->mb_width  , s->mb_height  , s->uvlinesize, 0);

        h_block_filter(s, s->current_picture.data[2], s->mb_width  , s->mb_height  , s->uvlinesize, 0);



        /* filter vertical block boundaries */

        v_block_filter(s, s->current_picture.data[0], s->mb_width*2, s->mb_height*2, s->linesize  , 1);

        v_block_filter(s, s->current_picture.data[1], s->mb_width  , s->mb_height  , s->uvlinesize, 0);

        v_block_filter(s, s->current_picture.data[2], s->mb_width  , s->mb_height  , s->uvlinesize, 0);

    }



#if CONFIG_MPEG_XVMC_DECODER

ec_clean:

#endif

    /* clean a few tables */

    for(i=0; i<s->mb_num; i++){

        const int mb_xy= s->mb_index2xy[i];

        int error= s->error_status_table[mb_xy];



        if(s->pict_type!=FF_B_TYPE && (error&(DC_ERROR|MV_ERROR|AC_ERROR))){

            s->mbskip_table[mb_xy]=0;

        }

        s->mbintra_table[mb_xy]=1;

    }

}
