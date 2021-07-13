av_cold int ff_mss12_decode_init(MSS12Context *c, int version,

                                 SliceContext* sc1, SliceContext *sc2)

{

    AVCodecContext *avctx = c->avctx;

    int i;



    if (avctx->extradata_size < 52 + 256 * 3) {

        av_log(avctx, AV_LOG_ERROR, "Insufficient extradata size %d\n",

               avctx->extradata_size);





    if (AV_RB32(avctx->extradata) < avctx->extradata_size) {

        av_log(avctx, AV_LOG_ERROR,

               "Insufficient extradata size: expected %d got %d\n",

               AV_RB32(avctx->extradata),

               avctx->extradata_size);





    avctx->coded_width  = AV_RB32(avctx->extradata + 20);

    avctx->coded_height = AV_RB32(avctx->extradata + 24);

    if (avctx->coded_width > 4096 || avctx->coded_height > 4096) {

        av_log(avctx, AV_LOG_ERROR, "Frame dimensions %dx%d too large",











    av_log(avctx, AV_LOG_DEBUG, "Encoder version %d.%d\n",

           AV_RB32(avctx->extradata + 4), AV_RB32(avctx->extradata + 8));

    if (version != AV_RB32(avctx->extradata + 4) > 1) {

        av_log(avctx, AV_LOG_ERROR,

               "Header version doesn't match codec tag\n");

        return -1;




    c->free_colours = AV_RB32(avctx->extradata + 48);

    if ((unsigned)c->free_colours > 256) {

        av_log(avctx, AV_LOG_ERROR,

               "Incorrect number of changeable palette entries: %d\n",

               c->free_colours);



    av_log(avctx, AV_LOG_DEBUG, "%d free colour(s)\n", c->free_colours);



    av_log(avctx, AV_LOG_DEBUG, "Display dimensions %dx%d\n",

           AV_RB32(avctx->extradata + 12), AV_RB32(avctx->extradata + 16));

    av_log(avctx, AV_LOG_DEBUG, "Coded dimensions %dx%d\n",


    av_log(avctx, AV_LOG_DEBUG, "%g frames per second\n",

           av_int2float(AV_RB32(avctx->extradata + 28)));

    av_log(avctx, AV_LOG_DEBUG, "Bitrate %d bps\n",

           AV_RB32(avctx->extradata + 32));

    av_log(avctx, AV_LOG_DEBUG, "Max. lead time %g ms\n",

           av_int2float(AV_RB32(avctx->extradata + 36)));

    av_log(avctx, AV_LOG_DEBUG, "Max. lag time %g ms\n",

           av_int2float(AV_RB32(avctx->extradata + 40)));

    av_log(avctx, AV_LOG_DEBUG, "Max. seek time %g ms\n",

           av_int2float(AV_RB32(avctx->extradata + 44)));



    if (version) {

        if (avctx->extradata_size < 60 + 256 * 3) {

            av_log(avctx, AV_LOG_ERROR,

                   "Insufficient extradata size %d for v2\n",

                   avctx->extradata_size);





        c->slice_split = AV_RB32(avctx->extradata + 52);

        av_log(avctx, AV_LOG_DEBUG, "Slice split %d\n", c->slice_split);



        c->full_model_syms = AV_RB32(avctx->extradata + 56);

        if (c->full_model_syms < 2 || c->full_model_syms > 256) {

            av_log(avctx, AV_LOG_ERROR,

                   "Incorrect number of used colours %d\n",

                   c->full_model_syms);



        av_log(avctx, AV_LOG_DEBUG, "Used colours %d\n",

               c->full_model_syms);

    } else {

        c->slice_split     = 0;

        c->full_model_syms = 256;




    for (i = 0; i < 256; i++)

        c->pal[i] = 0xFFU << 24 | AV_RB24(avctx->extradata + 52 +

                            (version ? 8 : 0) + i * 3);



    c->mask_stride = FFALIGN(avctx->width, 16);

    c->mask        = av_malloc(c->mask_stride * avctx->height);

    if (!c->mask) {

        av_log(avctx, AV_LOG_ERROR, "Cannot allocate mask plane\n");

        return AVERROR(ENOMEM);




    sc1->c = c;

    slicecontext_init(sc1, version, c->full_model_syms);

    if (c->slice_split) {

        sc2->c = c;

        slicecontext_init(sc2, version, c->full_model_syms);


    c->corrupted = 1;



    return 0;
