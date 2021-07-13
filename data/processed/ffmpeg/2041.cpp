static inline void MPV_motion_lowres(MpegEncContext *s,

                              uint8_t *dest_y, uint8_t *dest_cb, uint8_t *dest_cr,

                              int dir, uint8_t **ref_picture,

                              h264_chroma_mc_func *pix_op)

{

    int mx, my;

    int mb_x, mb_y, i;

    const int lowres= s->avctx->lowres;

    const int block_s= 8>>lowres;



    mb_x = s->mb_x;

    mb_y = s->mb_y;



    switch(s->mv_type) {

    case MV_TYPE_16X16:

        mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,

                    0, 0, 0,

                    ref_picture, pix_op,

                    s->mv[dir][0][0], s->mv[dir][0][1], 2*block_s);

        break;

    case MV_TYPE_8X8:

        mx = 0;

        my = 0;

            for(i=0;i<4;i++) {

                hpel_motion_lowres(s, dest_y + ((i & 1) + (i >> 1) * s->linesize)*block_s,

                            ref_picture[0], 0, 0,

                            (2*mb_x + (i & 1))*block_s, (2*mb_y + (i >>1))*block_s,

                            s->width, s->height, s->linesize,

                            s->h_edge_pos >> lowres, s->v_edge_pos >> lowres,

                            block_s, block_s, pix_op,

                            s->mv[dir][i][0], s->mv[dir][i][1]);



                mx += s->mv[dir][i][0];

                my += s->mv[dir][i][1];

            }



        if(!(s->flags&CODEC_FLAG_GRAY))

            chroma_4mv_motion_lowres(s, dest_cb, dest_cr, ref_picture, pix_op, mx, my);

        break;

    case MV_TYPE_FIELD:

        if (s->picture_structure == PICT_FRAME) {

            /* top field */

            mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,

                        1, 0, s->field_select[dir][0],

                        ref_picture, pix_op,

                        s->mv[dir][0][0], s->mv[dir][0][1], block_s);

            /* bottom field */

            mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,

                        1, 1, s->field_select[dir][1],

                        ref_picture, pix_op,

                        s->mv[dir][1][0], s->mv[dir][1][1], block_s);

        } else {

            if(s->picture_structure != s->field_select[dir][0] + 1 && s->pict_type != B_TYPE && !s->first_field){

                ref_picture= s->current_picture_ptr->data;

            }



            mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,

                        0, 0, s->field_select[dir][0],

                        ref_picture, pix_op,

                        s->mv[dir][0][0], s->mv[dir][0][1], 2*block_s);

        }

        break;

    case MV_TYPE_16X8:

        for(i=0; i<2; i++){

            uint8_t ** ref2picture;



            if(s->picture_structure == s->field_select[dir][i] + 1 || s->pict_type == B_TYPE || s->first_field){

                ref2picture= ref_picture;

            }else{

                ref2picture= s->current_picture_ptr->data;

            }



            mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,

                        0, 0, s->field_select[dir][i],

                        ref2picture, pix_op,

                        s->mv[dir][i][0], s->mv[dir][i][1] + 2*block_s*i, block_s);



            dest_y += 2*block_s*s->linesize;

            dest_cb+= (2*block_s>>s->chroma_y_shift)*s->uvlinesize;

            dest_cr+= (2*block_s>>s->chroma_y_shift)*s->uvlinesize;

        }

        break;

    case MV_TYPE_DMV:

        if(s->picture_structure == PICT_FRAME){

            for(i=0; i<2; i++){

                int j;

                for(j=0; j<2; j++){

                    mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,

                                1, j, j^i,

                                ref_picture, pix_op,

                                s->mv[dir][2*i + j][0], s->mv[dir][2*i + j][1], block_s);

                }

                pix_op = s->dsp.avg_h264_chroma_pixels_tab;

            }

        }else{

            for(i=0; i<2; i++){

                mpeg_motion_lowres(s, dest_y, dest_cb, dest_cr,

                            0, 0, s->picture_structure != i+1,

                            ref_picture, pix_op,

                            s->mv[dir][2*i][0],s->mv[dir][2*i][1],2*block_s);



                // after put we make avg of the same block

                pix_op = s->dsp.avg_h264_chroma_pixels_tab;



                //opposite parity is always in the same frame if this is second field

                if(!s->first_field){

                    ref_picture = s->current_picture_ptr->data;

                }

            }

        }

    break;

    default: assert(0);

    }

}
