static int read_matrix_params(MLPDecodeContext *m, SubStream *s, GetBitContext *gbp)

{

    unsigned int mat, ch;



    s->num_primitive_matrices = get_bits(gbp, 4);

    m->matrix_changed++;



    for (mat = 0; mat < s->num_primitive_matrices; mat++) {

        int frac_bits, max_chan;

        s->matrix_out_ch[mat] = get_bits(gbp, 4);

        frac_bits             = get_bits(gbp, 4);

        s->lsb_bypass   [mat] = get_bits1(gbp);



        if (s->matrix_out_ch[mat] > s->max_matrix_channel) {

            av_log(m->avctx, AV_LOG_ERROR,

                    "Invalid channel %d specified as output from matrix.\n",

                    s->matrix_out_ch[mat]);

            return -1;

        }

        if (frac_bits > 14) {

            av_log(m->avctx, AV_LOG_ERROR,

                    "Too many fractional bits specified.\n");

            return -1;

        }



        max_chan = s->max_matrix_channel;

        if (!s->noise_type)

            max_chan+=2;



        for (ch = 0; ch <= max_chan; ch++) {

            int coeff_val = 0;

            if (get_bits1(gbp))

                coeff_val = get_sbits(gbp, frac_bits + 2);



            s->matrix_coeff[mat][ch] = coeff_val << (14 - frac_bits);

        }



        if (s->noise_type)

            s->matrix_noise_shift[mat] = get_bits(gbp, 4);

        else

            s->matrix_noise_shift[mat] = 0;

    }



    return 0;

}
