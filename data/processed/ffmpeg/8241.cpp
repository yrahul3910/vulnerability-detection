void FUNC(ff_emulated_edge_mc)(uint8_t *buf, const uint8_t *src, int linesize, int block_w, int block_h,

                                    int src_x, int src_y, int w, int h){

    int x, y;

    int start_y, start_x, end_y, end_x;



    if(src_y>= h){

        src+= (h-1-src_y)*linesize;

        src_y=h-1;

    }else if(src_y<=-block_h){

        src+= (1-block_h-src_y)*linesize;

        src_y=1-block_h;

    }

    if(src_x>= w){

        src+= (w-1-src_x)*sizeof(pixel);

        src_x=w-1;

    }else if(src_x<=-block_w){

        src+= (1-block_w-src_x)*sizeof(pixel);

        src_x=1-block_w;

    }



    start_y= FFMAX(0, -src_y);

    start_x= FFMAX(0, -src_x);

    end_y= FFMIN(block_h, h-src_y);

    end_x= FFMIN(block_w, w-src_x);

    av_assert2(start_y < end_y && block_h);

    av_assert2(start_x < end_x && block_w);



    w    = end_x - start_x;

    src += start_y*linesize + start_x*sizeof(pixel);

    buf += start_x*sizeof(pixel);



    //top

    for(y=0; y<start_y; y++){

        memcpy(buf, src, w*sizeof(pixel));

        buf += linesize;

    }



    // copy existing part

    for(; y<end_y; y++){

        memcpy(buf, src, w*sizeof(pixel));

        src += linesize;

        buf += linesize;

    }



    //bottom

    src -= linesize;

    for(; y<block_h; y++){

        memcpy(buf, src, w*sizeof(pixel));

        buf += linesize;

    }



    buf -= block_h * linesize + start_x*sizeof(pixel);

    while (block_h--){

        pixel *bufp = (pixel*)buf;

       //left

        for(x=0; x<start_x; x++){

            bufp[x] = bufp[start_x];

        }



       //right

        for(x=end_x; x<block_w; x++){

            bufp[x] = bufp[end_x - 1];

        }

        buf += linesize;

    }

}
