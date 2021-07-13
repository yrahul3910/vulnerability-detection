static int theora_decode_header(AVCodecContext *avctx, GetBitContext gb)
{
    Vp3DecodeContext *s = avctx->priv_data;
    int major, minor, micro;
    major = get_bits(&gb, 8); /* version major */
    minor = get_bits(&gb, 8); /* version minor */
    micro = get_bits(&gb, 8); /* version micro */
    av_log(avctx, AV_LOG_INFO, "Theora bitstream version %d.%d.%d\n",
	major, minor, micro);
    /* FIXME: endianess? */
    s->theora = (major << 16) | (minor << 8) | micro;
    /* 3.3.0 aka alpha3 has the same frame orientation as original vp3 */
    /* but previous versions have the image flipped relative to vp3 */
    if (s->theora < 0x030300)
    {
	s->flipped_image = 1;
        av_log(avctx, AV_LOG_DEBUG, "Old (<alpha3) Theora bitstream, flipped image\n");
    s->width = get_bits(&gb, 16) << 4;
    s->height = get_bits(&gb, 16) << 4;
    skip_bits(&gb, 24); /* frame width */
    skip_bits(&gb, 24); /* frame height */
    skip_bits(&gb, 8); /* offset x */
    skip_bits(&gb, 8); /* offset y */
    skip_bits(&gb, 32); /* fps numerator */
    skip_bits(&gb, 32); /* fps denumerator */
    skip_bits(&gb, 24); /* aspect numerator */
    skip_bits(&gb, 24); /* aspect denumerator */
    if (s->theora < 0x030300)
	skip_bits(&gb, 5); /* keyframe frequency force */
    skip_bits(&gb, 8); /* colorspace */
    skip_bits(&gb, 24); /* bitrate */
    skip_bits(&gb, 6); /* last(?) quality index */
    if (s->theora >= 0x030300)
    {
	skip_bits(&gb, 5); /* keyframe frequency force */
	skip_bits(&gb, 5); /* spare bits */
//    align_get_bits(&gb);
    avctx->width = s->width;
    avctx->height = s->height;
    vp3_decode_init(avctx);
    return 0;