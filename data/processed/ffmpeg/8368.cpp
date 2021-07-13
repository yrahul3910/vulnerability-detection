static inline void halfpel_motion_search4(MpegEncContext * s,

				  int *mx_ptr, int *my_ptr, int dmin,

				  int xmin, int ymin, int xmax, int ymax,

                                  int pred_x, int pred_y, int block_x, int block_y,

                                  uint8_t *ref_picture)

{

    UINT16 *mv_penalty= s->mv_penalty[s->f_code] + MAX_MV; // f_code of the prev frame

    const int quant= s->qscale;

    int pen_x, pen_y;

    int mx, my, mx1, my1, d, xx, yy, dminh;

    UINT8 *pix, *ptr;



    xx = 8 * block_x;

    yy = 8 * block_y;

    pix =  s->new_picture[0] + (yy * s->linesize) + xx;

    

    mx = *mx_ptr;

    my = *my_ptr;

    ptr = ref_picture + ((yy+my) * s->linesize) + xx + mx;



    dminh = dmin;



    if (mx > xmin && mx < xmax && 

        my > ymin && my < ymax) {



        mx= mx1= 2*mx;

        my= my1= 2*my;

        if(dmin < Z_THRESHOLD && mx==0 && my==0){

            *mx_ptr = 0;

            *my_ptr = 0;

            return;

        }

        

        pen_x= pred_x + mx;

        pen_y= pred_y + my;



        ptr-= s->linesize;

        CHECK_HALF_MV4(xy2, -1, -1)

        CHECK_HALF_MV4(y2 ,  0, -1)

        CHECK_HALF_MV4(xy2, +1, -1)

        

        ptr+= s->linesize;

        CHECK_HALF_MV4(x2 , -1,  0)

        CHECK_HALF_MV4(x2 , +1,  0)

        CHECK_HALF_MV4(xy2, -1, +1)

        CHECK_HALF_MV4(y2 ,  0, +1)

        CHECK_HALF_MV4(xy2, +1, +1)



    }else{

        mx*=2;

        my*=2;

    }



    *mx_ptr = mx;

    *my_ptr = my;

}
