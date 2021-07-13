static int decode_hq_slice(AVCodecContext *avctx, void *arg)

{

    int i, quant, level, orientation, quant_idx;

    uint8_t quants[MAX_DWT_LEVELS][4];

    DiracContext *s = avctx->priv_data;

    DiracSlice *slice = arg;

    GetBitContext *gb = &slice->gb;



    skip_bits_long(gb, 8*s->highquality.prefix_bytes);

    quant_idx = get_bits(gb, 8);



    /* Slice quantization (slice_quantizers() in the specs) */

    for (level = 0; level < s->wavelet_depth; level++) {

        for (orientation = !!level; orientation < 4; orientation++) {

            quant = FFMAX(quant_idx - s->lowdelay.quant[level][orientation], 0);

            quants[level][orientation] = quant;

        }

    }



    /* Luma + 2 Chroma planes */

    for (i = 0; i < 3; i++) {

        int64_t length = s->highquality.size_scaler * get_bits(gb, 8);

        int64_t bits_left = 8 * length;

        int64_t bits_end = get_bits_count(gb) + bits_left;



        if (bits_end >= INT_MAX) {

            av_log(s->avctx, AV_LOG_ERROR, "end too far away\n");

            return AVERROR_INVALIDDATA;

        }



        for (level = 0; level < s->wavelet_depth; level++) {

            for (orientation = !!level; orientation < 4; orientation++) {

                decode_subband(s, gb, quants[level][orientation], slice->slice_x, slice->slice_y, bits_end,

                               &s->plane[i].band[level][orientation], NULL);

            }

        }

        skip_bits_long(gb, bits_end - get_bits_count(gb));

    }



    return 0;

}
