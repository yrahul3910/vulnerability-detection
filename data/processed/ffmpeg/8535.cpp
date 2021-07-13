static int decode_subframe(TAKDecContext *s, int32_t *decoded,

                           int subframe_size, int prev_subframe_size)

{

    GetBitContext *gb = &s->gb;

    int x, y, i, j, ret = 0;

    int dshift, size, filter_quant, filter_order, filter_order16;

    int tfilter[MAX_PREDICTORS];



    if (!get_bits1(gb))

        return decode_residues(s, decoded, subframe_size);



    filter_order = predictor_sizes[get_bits(gb, 4)];



    if (prev_subframe_size > 0 && get_bits1(gb)) {

        if (filter_order > prev_subframe_size)

            return AVERROR_INVALIDDATA;



        decoded       -= filter_order;

        subframe_size += filter_order;



        if (filter_order > subframe_size)

            return AVERROR_INVALIDDATA;

    } else {

        int lpc_mode;



        if (filter_order > subframe_size)

            return AVERROR_INVALIDDATA;



        lpc_mode = get_bits(gb, 2);

        if (lpc_mode > 2)

            return AVERROR_INVALIDDATA;



        if ((ret = decode_residues(s, decoded, filter_order)) < 0)

            return ret;



        if (lpc_mode)

            decode_lpc(decoded, lpc_mode, filter_order);

    }



    dshift = get_bits_esc4(gb);

    size   = get_bits1(gb) + 6;



    filter_quant = 10;

    if (get_bits1(gb)) {

        filter_quant -= get_bits(gb, 3) + 1;

        if (filter_quant < 3)

            return AVERROR_INVALIDDATA;

    }



    s->predictors[0] = get_sbits(gb, 10);

    s->predictors[1] = get_sbits(gb, 10);

    s->predictors[2] = get_sbits(gb, size) << (10 - size);

    s->predictors[3] = get_sbits(gb, size) << (10 - size);

    if (filter_order > 4) {

        int tmp = size - get_bits1(gb);



        for (i = 4; i < filter_order; i++) {

            if (!(i & 3))

                x = tmp - get_bits(gb, 2);

            s->predictors[i] = get_sbits(gb, x) << (10 - size);

        }

    }



    tfilter[0] = s->predictors[0] << 6;

    for (i = 1; i < filter_order; i++) {

        int32_t *p1 = &tfilter[0];

        int32_t *p2 = &tfilter[i - 1];



        for (j = 0; j < (i + 1) / 2; j++) {

            x     = *p1 + (s->predictors[i] * *p2 + 256 >> 9);

            *p2  += s->predictors[i] * *p1 + 256 >> 9;

            *p1++ = x;

            p2--;

        }



        tfilter[i] = s->predictors[i] << 6;

    }



    filter_order16 = FFALIGN(filter_order, 16);

    AV_ZERO128(s->filter + filter_order16 - 16);

    AV_ZERO128(s->filter + filter_order16 -  8);

    x = 1 << (32 - (15 - filter_quant));

    y = 1 << ((15 - filter_quant) - 1);

    for (i = 0, j = filter_order - 1; i < filter_order / 2; i++, j--) {

        s->filter[j] = x - ((tfilter[i] + y) >> (15 - filter_quant));

        s->filter[i] = x - ((tfilter[j] + y) >> (15 - filter_quant));

    }



    if ((ret = decode_residues(s, &decoded[filter_order],

                               subframe_size - filter_order)) < 0)

        return ret;



    for (i = 0; i < filter_order; i++)

        s->residues[i] = *decoded++ >> dshift;



    y    = FF_ARRAY_ELEMS(s->residues) - filter_order;

    x    = subframe_size - filter_order;

    while (x > 0) {

        int tmp = FFMIN(y, x);



        for (i = 0; i < tmp; i++) {

            int v = 1 << (filter_quant - 1);



            v += s->adsp.scalarproduct_int16(&s->residues[i], s->filter,

                                             filter_order16);

            v = (av_clip_intp2(v >> filter_quant, 13) << dshift) - *decoded;

            *decoded++ = v;

            s->residues[filter_order + i] = v >> dshift;

        }



        x -= tmp;

        if (x > 0)

            memcpy(s->residues, &s->residues[y], 2 * filter_order);

    }



    emms_c();



    return 0;

}
