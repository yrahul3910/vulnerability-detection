static void vp8_decode_mb_row_no_filter(AVCodecContext *avctx, void *tdata,

                                        int jobnr, int threadnr)

{

    decode_mb_row_no_filter(avctx, tdata, jobnr, threadnr, 0);

}
