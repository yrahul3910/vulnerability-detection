static void loop_filter(H264Context *h, H264SliceContext *sl, int start_x, int end_x)

{

    uint8_t *dest_y, *dest_cb, *dest_cr;

    int linesize, uvlinesize, mb_x, mb_y;

    const int end_mb_y       = h->mb_y + FRAME_MBAFF(h);

    const int old_slice_type = sl->slice_type;

    const int pixel_shift    = h->pixel_shift;

    const int block_h        = 16 >> h->chroma_y_shift;



    if (h->deblocking_filter) {

        for (mb_x = start_x; mb_x < end_x; mb_x++)

            for (mb_y = end_mb_y - FRAME_MBAFF(h); mb_y <= end_mb_y; mb_y++) {

                int mb_xy, mb_type;

                mb_xy         = h->mb_xy = mb_x + mb_y * h->mb_stride;

                sl->slice_num = h->slice_table[mb_xy];

                mb_type       = h->cur_pic.mb_type[mb_xy];

                sl->list_count = h->list_counts[mb_xy];



                if (FRAME_MBAFF(h))

                    h->mb_mbaff               =

                    h->mb_field_decoding_flag = !!IS_INTERLACED(mb_type);



                h->mb_x = mb_x;

                h->mb_y = mb_y;

                dest_y  = h->cur_pic.f.data[0] +

                          ((mb_x << pixel_shift) + mb_y * h->linesize) * 16;

                dest_cb = h->cur_pic.f.data[1] +

                          (mb_x << pixel_shift) * (8 << CHROMA444(h)) +

                          mb_y * h->uvlinesize * block_h;

                dest_cr = h->cur_pic.f.data[2] +

                          (mb_x << pixel_shift) * (8 << CHROMA444(h)) +

                          mb_y * h->uvlinesize * block_h;

                // FIXME simplify above



                if (MB_FIELD(h)) {

                    linesize   = sl->mb_linesize   = h->linesize   * 2;

                    uvlinesize = sl->mb_uvlinesize = h->uvlinesize * 2;

                    if (mb_y & 1) { // FIXME move out of this function?

                        dest_y  -= h->linesize   * 15;

                        dest_cb -= h->uvlinesize * (block_h - 1);

                        dest_cr -= h->uvlinesize * (block_h - 1);

                    }

                } else {

                    linesize   = sl->mb_linesize   = h->linesize;

                    uvlinesize = sl->mb_uvlinesize = h->uvlinesize;

                }

                backup_mb_border(h, dest_y, dest_cb, dest_cr, linesize,

                                 uvlinesize, 0);

                if (fill_filter_caches(h, sl, mb_type))

                    continue;

                sl->chroma_qp[0] = get_chroma_qp(h, 0, h->cur_pic.qscale_table[mb_xy]);

                sl->chroma_qp[1] = get_chroma_qp(h, 1, h->cur_pic.qscale_table[mb_xy]);



                if (FRAME_MBAFF(h)) {

                    ff_h264_filter_mb(h, sl, mb_x, mb_y, dest_y, dest_cb, dest_cr,

                                      linesize, uvlinesize);

                } else {

                    ff_h264_filter_mb_fast(h, sl, mb_x, mb_y, dest_y, dest_cb,

                                           dest_cr, linesize, uvlinesize);

                }

            }

    }

    sl->slice_type  = old_slice_type;

    h->mb_x         = end_x;

    h->mb_y         = end_mb_y - FRAME_MBAFF(h);

    sl->chroma_qp[0] = get_chroma_qp(h, 0, sl->qscale);

    sl->chroma_qp[1] = get_chroma_qp(h, 1, sl->qscale);

}
