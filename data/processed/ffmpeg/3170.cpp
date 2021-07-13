static int read_extra_header(FFV1Context *f)

{

    RangeCoder *const c = &f->c;

    uint8_t state[CONTEXT_SIZE];

    int i, j, k, ret;

    uint8_t state2[32][CONTEXT_SIZE];



    memset(state2, 128, sizeof(state2));

    memset(state, 128, sizeof(state));



    ff_init_range_decoder(c, f->avctx->extradata, f->avctx->extradata_size);

    ff_build_rac_states(c, 0.05 * (1LL << 32), 256 - 8);



    f->version = get_symbol(c, state, 0);

    if (f->version < 2) {

        av_log(f->avctx, AV_LOG_ERROR, "Invalid version in global header\n");

        return AVERROR_INVALIDDATA;

    }

    if (f->version > 2) {

        c->bytestream_end -= 4;

        f->micro_version = get_symbol(c, state, 0);

        if (f->micro_version < 0)

            return AVERROR_INVALIDDATA;

    }

    f->ac = f->avctx->coder_type = get_symbol(c, state, 0);

    if (f->ac > 1) {

        for (i = 1; i < 256; i++)

            f->state_transition[i] = get_symbol(c, state, 1) + c->one_state[i];

    }



    f->colorspace                 = get_symbol(c, state, 0); //YUV cs type

    f->avctx->bits_per_raw_sample = get_symbol(c, state, 0);

    f->chroma_planes              = get_rac(c, state);

    f->chroma_h_shift             = get_symbol(c, state, 0);

    f->chroma_v_shift             = get_symbol(c, state, 0);

    f->transparency               = get_rac(c, state);

    f->plane_count                = 1 + (f->chroma_planes || f->version<4) + f->transparency;

    f->num_h_slices               = 1 + get_symbol(c, state, 0);

    f->num_v_slices               = 1 + get_symbol(c, state, 0);



    if (f->chroma_h_shift > 4U || f->chroma_v_shift > 4U) {

        av_log(f->avctx, AV_LOG_ERROR, "chroma shift parameters %d %d are invalid\n",

               f->chroma_h_shift, f->chroma_v_shift);

        return AVERROR_INVALIDDATA;

    }



    if (f->num_h_slices > (unsigned)f->width  || !f->num_h_slices ||

        f->num_v_slices > (unsigned)f->height || !f->num_v_slices

       ) {

        av_log(f->avctx, AV_LOG_ERROR, "slice count invalid\n");

        return AVERROR_INVALIDDATA;

    }



    f->quant_table_count = get_symbol(c, state, 0);

    if (f->quant_table_count > (unsigned)MAX_QUANT_TABLES)

        return AVERROR_INVALIDDATA;



    for (i = 0; i < f->quant_table_count; i++) {

        f->context_count[i] = read_quant_tables(c, f->quant_tables[i]);

        if (f->context_count[i] < 0) {

            av_log(f->avctx, AV_LOG_ERROR, "read_quant_table error\n");

            return AVERROR_INVALIDDATA;

        }

    }

    if ((ret = ff_ffv1_allocate_initial_states(f)) < 0)

        return ret;



    for (i = 0; i < f->quant_table_count; i++)

        if (get_rac(c, state)) {

            for (j = 0; j < f->context_count[i]; j++)

                for (k = 0; k < CONTEXT_SIZE; k++) {

                    int pred = j ? f->initial_states[i][j - 1][k] : 128;

                    f->initial_states[i][j][k] =

                        (pred + get_symbol(c, state2[k], 1)) & 0xFF;

                }

        }



    if (f->version > 2) {

        f->ec = get_symbol(c, state, 0);

        if (f->micro_version > 2)

            f->intra = get_symbol(c, state, 0);

    }



    if (f->version > 2) {

        unsigned v;

        v = av_crc(av_crc_get_table(AV_CRC_32_IEEE), 0,

                   f->avctx->extradata, f->avctx->extradata_size);

        if (v) {

            av_log(f->avctx, AV_LOG_ERROR, "CRC mismatch %X!\n", v);

            return AVERROR_INVALIDDATA;

        }

    }



    if (f->avctx->debug & FF_DEBUG_PICT_INFO)

        av_log(f->avctx, AV_LOG_DEBUG,

               "global: ver:%d.%d, coder:%d, colorspace: %d bpr:%d chroma:%d(%d:%d), alpha:%d slices:%dx%d qtabs:%d ec:%d intra:%d\n",

               f->version, f->micro_version,

               f->ac,

               f->colorspace,

               f->avctx->bits_per_raw_sample,

               f->chroma_planes, f->chroma_h_shift, f->chroma_v_shift,

               f->transparency,

               f->num_h_slices, f->num_v_slices,

               f->quant_table_count,

               f->ec,

               f->intra

              );

    return 0;

}
