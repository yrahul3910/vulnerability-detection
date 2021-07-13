static void render_slice(Vp3DecodeContext *s, int slice)

{

    int x, y;

    int m, n;

    int i;  /* indicates current fragment */

    int16_t *dequantizer;

    DCTELEM __align16 block[64];

    unsigned char *output_plane;

    unsigned char *last_plane;

    unsigned char *golden_plane;

    int stride;

    int motion_x = 0xdeadbeef, motion_y = 0xdeadbeef;

    int upper_motion_limit, lower_motion_limit;

    int motion_halfpel_index;

    uint8_t *motion_source;

    int plane;

    int plane_width;

    int plane_height;

    int slice_height;

    int current_macroblock_entry = slice * s->macroblock_width * 6;



    if (slice >= s->macroblock_height)

        return;



    for (plane = 0; plane < 3; plane++) {



        /* set up plane-specific parameters */

        if (plane == 0) {

            output_plane = s->current_frame.data[0];

            last_plane = s->last_frame.data[0];

            golden_plane = s->golden_frame.data[0];

            stride = s->current_frame.linesize[0];

            if (!s->flipped_image) stride = -stride;

            upper_motion_limit = 7 * s->current_frame.linesize[0];

            lower_motion_limit = s->height * s->current_frame.linesize[0] + s->width - 8;

            y = slice * FRAGMENT_PIXELS * 2;

            plane_width = s->width;

            plane_height = s->height;

            slice_height = y + FRAGMENT_PIXELS * 2;

            i = s->macroblock_fragments[current_macroblock_entry + 0];

        } else if (plane == 1) {

            output_plane = s->current_frame.data[1];

            last_plane = s->last_frame.data[1];

            golden_plane = s->golden_frame.data[1];

            stride = s->current_frame.linesize[1];

            if (!s->flipped_image) stride = -stride;

            upper_motion_limit = 7 * s->current_frame.linesize[1];

            lower_motion_limit = (s->height / 2) * s->current_frame.linesize[1] + (s->width / 2) - 8;

            y = slice * FRAGMENT_PIXELS;

            plane_width = s->width / 2;

            plane_height = s->height / 2;

            slice_height = y + FRAGMENT_PIXELS;

            i = s->macroblock_fragments[current_macroblock_entry + 4];

        } else {

            output_plane = s->current_frame.data[2];

            last_plane = s->last_frame.data[2];

            golden_plane = s->golden_frame.data[2];

            stride = s->current_frame.linesize[2];

            if (!s->flipped_image) stride = -stride;

            upper_motion_limit = 7 * s->current_frame.linesize[2];

            lower_motion_limit = (s->height / 2) * s->current_frame.linesize[2] + (s->width / 2) - 8;

            y = slice * FRAGMENT_PIXELS;

            plane_width = s->width / 2;

            plane_height = s->height / 2;

            slice_height = y + FRAGMENT_PIXELS;

            i = s->macroblock_fragments[current_macroblock_entry + 5];

        }

    

        if(ABS(stride) > 2048)

            return; //various tables are fixed size



        /* for each fragment row in the slice (both of them)... */

        for (; y < slice_height; y += 8) {



            /* for each fragment in a row... */

            for (x = 0; x < plane_width; x += 8, i++) {



                if ((i < 0) || (i >= s->fragment_count)) {

                    av_log(s->avctx, AV_LOG_ERROR, "  vp3:render_slice(): bad fragment number (%d)\n", i);

                    return;

                }



                /* transform if this block was coded */

                if ((s->all_fragments[i].coding_method != MODE_COPY) &&

                    !((s->avctx->flags & CODEC_FLAG_GRAY) && plane)) {



                    if ((s->all_fragments[i].coding_method == MODE_USING_GOLDEN) ||

                        (s->all_fragments[i].coding_method == MODE_GOLDEN_MV))

                        motion_source= golden_plane;

                    else 

                        motion_source= last_plane;



                    motion_source += s->all_fragments[i].first_pixel;

                    motion_halfpel_index = 0;



                    /* sort out the motion vector if this fragment is coded

                     * using a motion vector method */

                    if ((s->all_fragments[i].coding_method > MODE_INTRA) &&

                        (s->all_fragments[i].coding_method != MODE_USING_GOLDEN)) {

                        int src_x, src_y;

                        motion_x = s->all_fragments[i].motion_x;

                        motion_y = s->all_fragments[i].motion_y;

                        if(plane){

                            motion_x= (motion_x>>1) | (motion_x&1);

                            motion_y= (motion_y>>1) | (motion_y&1);

                        }



                        src_x= (motion_x>>1) + x;

                        src_y= (motion_y>>1) + y;

                        if ((motion_x == 127) || (motion_y == 127))

                            av_log(s->avctx, AV_LOG_ERROR, " help! got invalid motion vector! (%X, %X)\n", motion_x, motion_y);



                        motion_halfpel_index = motion_x & 0x01;

                        motion_source += (motion_x >> 1);



                        motion_halfpel_index |= (motion_y & 0x01) << 1;

                        motion_source += ((motion_y >> 1) * stride);



                        if(src_x<0 || src_y<0 || src_x + 9 >= plane_width || src_y + 9 >= plane_height){

                            uint8_t *temp= s->edge_emu_buffer;

                            if(stride<0) temp -= 9*stride;

                            else temp += 9*stride;



                            ff_emulated_edge_mc(temp, motion_source, stride, 9, 9, src_x, src_y, plane_width, plane_height);

                            motion_source= temp;

                        }

                    }

                



                    /* first, take care of copying a block from either the

                     * previous or the golden frame */

                    if (s->all_fragments[i].coding_method != MODE_INTRA) {

                        /* Note, it is possible to implement all MC cases with 

                           put_no_rnd_pixels_l2 which would look more like the 

                           VP3 source but this would be slower as 

                           put_no_rnd_pixels_tab is better optimzed */

                        if(motion_halfpel_index != 3){

                            s->dsp.put_no_rnd_pixels_tab[1][motion_halfpel_index](

                                output_plane + s->all_fragments[i].first_pixel,

                                motion_source, stride, 8);

                        }else{

                            int d= (motion_x ^ motion_y)>>31; // d is 0 if motion_x and _y have the same sign, else -1

                            s->dsp.put_no_rnd_pixels_l2[1](

                                output_plane + s->all_fragments[i].first_pixel,

                                motion_source - d, 

                                motion_source + stride + 1 + d, 

                                stride, 8);

                        }

                        dequantizer = s->inter_dequant;

                    }else{

                        if (plane == 0)

                            dequantizer = s->intra_y_dequant;

                        else

                            dequantizer = s->intra_c_dequant;

                    }



                    /* dequantize the DCT coefficients */

                    debug_idct("fragment %d, coding mode %d, DC = %d, dequant = %d:\n", 

                        i, s->all_fragments[i].coding_method, 

                        DC_COEFF(i), dequantizer[0]);



                    if(s->avctx->idct_algo==FF_IDCT_VP3){

                        Coeff *coeff= s->coeffs + i;

                        memset(block, 0, sizeof(block));

                        while(coeff->next){

                            block[coeff->index]= coeff->coeff * dequantizer[coeff->index];

                            coeff= coeff->next;

                        }

                    }else{

                        Coeff *coeff= s->coeffs + i;

                        memset(block, 0, sizeof(block));

                        while(coeff->next){

                            block[coeff->index]= (coeff->coeff * dequantizer[coeff->index] + 2)>>2;

                            coeff= coeff->next;

                        }

                    }



                    /* invert DCT and place (or add) in final output */

                

                    if (s->all_fragments[i].coding_method == MODE_INTRA) {

                        if(s->avctx->idct_algo!=FF_IDCT_VP3)

                            block[0] += 128<<3;

                        s->dsp.idct_put(

                            output_plane + s->all_fragments[i].first_pixel,

                            stride,

                            block);

                    } else {

                        s->dsp.idct_add(

                            output_plane + s->all_fragments[i].first_pixel,

                            stride,

                            block);

                    }



                    debug_idct("block after idct_%s():\n",

                        (s->all_fragments[i].coding_method == MODE_INTRA)?

                        "put" : "add");

                    for (m = 0; m < 8; m++) {

                        for (n = 0; n < 8; n++) {

                            debug_idct(" %3d", *(output_plane + 

                                s->all_fragments[i].first_pixel + (m * stride + n)));

                        }

                        debug_idct("\n");

                    }

                    debug_idct("\n");



                } else {



                    /* copy directly from the previous frame */

                    s->dsp.put_pixels_tab[1][0](

                        output_plane + s->all_fragments[i].first_pixel,

                        last_plane + s->all_fragments[i].first_pixel,

                        stride, 8);



                }

            }

        }

    }



    /* future loop filter logic goes here... */

    /* algorithm: 

     *   if (slice != 0) 

     *     run filter on 1st row of Y slice

     *     run filter on U slice

     *     run filter on V slice

     *   run filter on 2nd row of Y slice

     */



     /* this looks like a good place for slice dispatch... */

     /* algorithm:

      *   if (slice > 0)

      *     dispatch (slice - 1);

      *   if (slice == s->macroblock_height - 1)

      *     dispatch (slice);  // handle last slice

      */



    emms_c();

}
