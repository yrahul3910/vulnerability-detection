static av_cold int pnm_encode_close(AVCodecContext *avctx)

{

    av_frame_free(&avctx->coded_frame);

    return 0;

}
