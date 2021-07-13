static int wma_decode_block(WMADecodeContext *s)

{

    int n, v, a, ch, code, bsize;

    int coef_nb_bits, total_gain, parse_exponents;

    DECLARE_ALIGNED_16(float, window[BLOCK_MAX_SIZE * 2]);

    int nb_coefs[MAX_CHANNELS];

    float mdct_norm;



#ifdef TRACE

    tprintf("***decode_block: %d:%d\n", s->frame_count - 1, s->block_num);

#endif



    /* compute current block length */

    if (s->use_variable_block_len) {

        n = av_log2(s->nb_block_sizes - 1) + 1;



        if (s->reset_block_lengths) {

            s->reset_block_lengths = 0;

            v = get_bits(&s->gb, n);

            if (v >= s->nb_block_sizes)

                return -1;

            s->prev_block_len_bits = s->frame_len_bits - v;

            v = get_bits(&s->gb, n);

            if (v >= s->nb_block_sizes)

                return -1;

            s->block_len_bits = s->frame_len_bits - v;

        } else {

            /* update block lengths */

            s->prev_block_len_bits = s->block_len_bits;

            s->block_len_bits = s->next_block_len_bits;

        }

        v = get_bits(&s->gb, n);

        if (v >= s->nb_block_sizes)

            return -1;

        s->next_block_len_bits = s->frame_len_bits - v;

    } else {

        /* fixed block len */

        s->next_block_len_bits = s->frame_len_bits;

        s->prev_block_len_bits = s->frame_len_bits;

        s->block_len_bits = s->frame_len_bits;

    }



    /* now check if the block length is coherent with the frame length */

    s->block_len = 1 << s->block_len_bits;

    if ((s->block_pos + s->block_len) > s->frame_len)

        return -1;



    if (s->nb_channels == 2) {

        s->ms_stereo = get_bits(&s->gb, 1);

    }

    v = 0;

    for(ch = 0; ch < s->nb_channels; ch++) {

        a = get_bits(&s->gb, 1);

        s->channel_coded[ch] = a;

        v |= a;

    }

    /* if no channel coded, no need to go further */

    /* XXX: fix potential framing problems */

    if (!v)

        goto next;



    bsize = s->frame_len_bits - s->block_len_bits;



    /* read total gain and extract corresponding number of bits for

       coef escape coding */

    total_gain = 1;

    for(;;) {

        a = get_bits(&s->gb, 7);

        total_gain += a;

        if (a != 127)

            break;

    }



    if (total_gain < 15)

        coef_nb_bits = 13;

    else if (total_gain < 32)

        coef_nb_bits = 12;

    else if (total_gain < 40)

        coef_nb_bits = 11;

    else if (total_gain < 45)

        coef_nb_bits = 10;

    else

        coef_nb_bits = 9;



    /* compute number of coefficients */

    n = s->coefs_end[bsize] - s->coefs_start;

    for(ch = 0; ch < s->nb_channels; ch++)

        nb_coefs[ch] = n;



    /* complex coding */

    if (s->use_noise_coding) {



        for(ch = 0; ch < s->nb_channels; ch++) {

            if (s->channel_coded[ch]) {

                int i, n, a;

                n = s->exponent_high_sizes[bsize];

                for(i=0;i<n;i++) {

                    a = get_bits(&s->gb, 1);

                    s->high_band_coded[ch][i] = a;

                    /* if noise coding, the coefficients are not transmitted */

                    if (a)

                        nb_coefs[ch] -= s->exponent_high_bands[bsize][i];

                }

            }

        }

        for(ch = 0; ch < s->nb_channels; ch++) {

            if (s->channel_coded[ch]) {

                int i, n, val, code;



                n = s->exponent_high_sizes[bsize];

                val = (int)0x80000000;

                for(i=0;i<n;i++) {

                    if (s->high_band_coded[ch][i]) {

                        if (val == (int)0x80000000) {

                            val = get_bits(&s->gb, 7) - 19;

                        } else {

                            code = get_vlc2(&s->gb, s->hgain_vlc.table, HGAINVLCBITS, HGAINMAX);

                            if (code < 0)

                                return -1;

                            val += code - 18;

                        }

                        s->high_band_values[ch][i] = val;

                    }

                }

            }

        }

    }



    /* exposant can be interpolated in short blocks. */

    parse_exponents = 1;

    if (s->block_len_bits != s->frame_len_bits) {

        parse_exponents = get_bits(&s->gb, 1);

    }



    if (parse_exponents) {

        for(ch = 0; ch < s->nb_channels; ch++) {

            if (s->channel_coded[ch]) {

                if (s->use_exp_vlc) {

                    if (decode_exp_vlc(s, ch) < 0)

                        return -1;

                } else {

                    decode_exp_lsp(s, ch);

                }

            }

        }

    } else {

        for(ch = 0; ch < s->nb_channels; ch++) {

            if (s->channel_coded[ch]) {

                interpolate_array(s->exponents[ch], 1 << s->prev_block_len_bits,

                                  s->block_len);

            }

        }

    }



    /* parse spectral coefficients : just RLE encoding */

    for(ch = 0; ch < s->nb_channels; ch++) {

        if (s->channel_coded[ch]) {

            VLC *coef_vlc;

            int level, run, sign, tindex;

            int16_t *ptr, *eptr;

            const uint16_t *level_table, *run_table;



            /* special VLC tables are used for ms stereo because

               there is potentially less energy there */

            tindex = (ch == 1 && s->ms_stereo);

            coef_vlc = &s->coef_vlc[tindex];

            run_table = s->run_table[tindex];

            level_table = s->level_table[tindex];

            /* XXX: optimize */

            ptr = &s->coefs1[ch][0];

            eptr = ptr + nb_coefs[ch];

            memset(ptr, 0, s->block_len * sizeof(int16_t));

            for(;;) {

                code = get_vlc2(&s->gb, coef_vlc->table, VLCBITS, VLCMAX);

                if (code < 0)

                    return -1;

                if (code == 1) {

                    /* EOB */

                    break;

                } else if (code == 0) {

                    /* escape */

                    level = get_bits(&s->gb, coef_nb_bits);

                    /* NOTE: this is rather suboptimal. reading

                       block_len_bits would be better */

                    run = get_bits(&s->gb, s->frame_len_bits);

                } else {

                    /* normal code */

                    run = run_table[code];

                    level = level_table[code];

                }

                sign = get_bits(&s->gb, 1);

                if (!sign)

                    level = -level;

                ptr += run;

                if (ptr >= eptr)

                {

                    av_log(NULL, AV_LOG_ERROR, "overflow in spectral RLE, ignoring\n");

                    break;

                }

                *ptr++ = level;

                /* NOTE: EOB can be omitted */

                if (ptr >= eptr)

                    break;

            }

        }

        if (s->version == 1 && s->nb_channels >= 2) {

            align_get_bits(&s->gb);

        }

    }



    /* normalize */

    {

        int n4 = s->block_len / 2;

        mdct_norm = 1.0 / (float)n4;

        if (s->version == 1) {

            mdct_norm *= sqrt(n4);

        }

    }



    /* finally compute the MDCT coefficients */

    for(ch = 0; ch < s->nb_channels; ch++) {

        if (s->channel_coded[ch]) {

            int16_t *coefs1;

            float *coefs, *exponents, mult, mult1, noise, *exp_ptr;

            int i, j, n, n1, last_high_band;

            float exp_power[HIGH_BAND_MAX_SIZE];



            coefs1 = s->coefs1[ch];

            exponents = s->exponents[ch];

            mult = pow(10, total_gain * 0.05) / s->max_exponent[ch];

            mult *= mdct_norm;

            coefs = s->coefs[ch];

            if (s->use_noise_coding) {

                mult1 = mult;

                /* very low freqs : noise */

                for(i = 0;i < s->coefs_start; i++) {

                    *coefs++ = s->noise_table[s->noise_index] * (*exponents++) * mult1;

                    s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);

                }



                n1 = s->exponent_high_sizes[bsize];



                /* compute power of high bands */

                exp_ptr = exponents +

                    s->high_band_start[bsize] -

                    s->coefs_start;

                last_high_band = 0; /* avoid warning */

                for(j=0;j<n1;j++) {

                    n = s->exponent_high_bands[s->frame_len_bits -

                                              s->block_len_bits][j];

                    if (s->high_band_coded[ch][j]) {

                        float e2, v;

                        e2 = 0;

                        for(i = 0;i < n; i++) {

                            v = exp_ptr[i];

                            e2 += v * v;

                        }

                        exp_power[j] = e2 / n;

                        last_high_band = j;

                        tprintf("%d: power=%f (%d)\n", j, exp_power[j], n);

                    }

                    exp_ptr += n;

                }



                /* main freqs and high freqs */

                for(j=-1;j<n1;j++) {

                    if (j < 0) {

                        n = s->high_band_start[bsize] -

                            s->coefs_start;

                    } else {

                        n = s->exponent_high_bands[s->frame_len_bits -

                                                  s->block_len_bits][j];

                    }

                    if (j >= 0 && s->high_band_coded[ch][j]) {

                        /* use noise with specified power */

                        mult1 = sqrt(exp_power[j] / exp_power[last_high_band]);

                        /* XXX: use a table */

                        mult1 = mult1 * pow(10, s->high_band_values[ch][j] * 0.05);

                        mult1 = mult1 / (s->max_exponent[ch] * s->noise_mult);

                        mult1 *= mdct_norm;

                        for(i = 0;i < n; i++) {

                            noise = s->noise_table[s->noise_index];

                            s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);

                            *coefs++ = (*exponents++) * noise * mult1;

                        }

                    } else {

                        /* coded values + small noise */

                        for(i = 0;i < n; i++) {

                            noise = s->noise_table[s->noise_index];

                            s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);

                            *coefs++ = ((*coefs1++) + noise) * (*exponents++) * mult;

                        }

                    }

                }



                /* very high freqs : noise */

                n = s->block_len - s->coefs_end[bsize];

                mult1 = mult * exponents[-1];

                for(i = 0; i < n; i++) {

                    *coefs++ = s->noise_table[s->noise_index] * mult1;

                    s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);

                }

            } else {

                /* XXX: optimize more */

                for(i = 0;i < s->coefs_start; i++)

                    *coefs++ = 0.0;

                n = nb_coefs[ch];

                for(i = 0;i < n; i++) {

                    *coefs++ = coefs1[i] * exponents[i] * mult;

                }

                n = s->block_len - s->coefs_end[bsize];

                for(i = 0;i < n; i++)

                    *coefs++ = 0.0;

            }

        }

    }



#ifdef TRACE

    for(ch = 0; ch < s->nb_channels; ch++) {

        if (s->channel_coded[ch]) {

            dump_floats("exponents", 3, s->exponents[ch], s->block_len);

            dump_floats("coefs", 1, s->coefs[ch], s->block_len);

        }

    }

#endif



    if (s->ms_stereo && s->channel_coded[1]) {

        float a, b;

        int i;



        /* nominal case for ms stereo: we do it before mdct */

        /* no need to optimize this case because it should almost

           never happen */

        if (!s->channel_coded[0]) {

            tprintf("rare ms-stereo case happened\n");

            memset(s->coefs[0], 0, sizeof(float) * s->block_len);

            s->channel_coded[0] = 1;

        }



        for(i = 0; i < s->block_len; i++) {

            a = s->coefs[0][i];

            b = s->coefs[1][i];

            s->coefs[0][i] = a + b;

            s->coefs[1][i] = a - b;

        }

    }



    /* build the window : we ensure that when the windows overlap

       their squared sum is always 1 (MDCT reconstruction rule) */

    /* XXX: merge with output */

    {

        int i, next_block_len, block_len, prev_block_len, n;

        float *wptr;



        block_len = s->block_len;

        prev_block_len = 1 << s->prev_block_len_bits;

        next_block_len = 1 << s->next_block_len_bits;



        /* right part */

        wptr = window + block_len;

        if (block_len <= next_block_len) {

            for(i=0;i<block_len;i++)

                *wptr++ = s->windows[bsize][i];

        } else {

            /* overlap */

            n = (block_len / 2) - (next_block_len / 2);

            for(i=0;i<n;i++)

                *wptr++ = 1.0;

            for(i=0;i<next_block_len;i++)

                *wptr++ = s->windows[s->frame_len_bits - s->next_block_len_bits][i];

            for(i=0;i<n;i++)

                *wptr++ = 0.0;

        }



        /* left part */

        wptr = window + block_len;

        if (block_len <= prev_block_len) {

            for(i=0;i<block_len;i++)

                *--wptr = s->windows[bsize][i];

        } else {

            /* overlap */

            n = (block_len / 2) - (prev_block_len / 2);

            for(i=0;i<n;i++)

                *--wptr = 1.0;

            for(i=0;i<prev_block_len;i++)

                *--wptr = s->windows[s->frame_len_bits - s->prev_block_len_bits][i];

            for(i=0;i<n;i++)

                *--wptr = 0.0;

        }

    }





    for(ch = 0; ch < s->nb_channels; ch++) {

        if (s->channel_coded[ch]) {

            DECLARE_ALIGNED_16(FFTSample, output[BLOCK_MAX_SIZE * 2]);

            float *ptr;

            int n4, index, n;



            n = s->block_len;

            n4 = s->block_len / 2;

            s->mdct_ctx[bsize].fft.imdct_calc(&s->mdct_ctx[bsize],

                          output, s->coefs[ch], s->mdct_tmp);



            /* XXX: optimize all that by build the window and

               multipying/adding at the same time */



            /* multiply by the window and add in the frame */

            index = (s->frame_len / 2) + s->block_pos - n4;

            ptr = &s->frame_out[ch][index];

            s->dsp.vector_fmul_add_add(ptr,window,output,ptr,0,2*n,1);



            /* specific fast case for ms-stereo : add to second

               channel if it is not coded */

            if (s->ms_stereo && !s->channel_coded[1]) {

                ptr = &s->frame_out[1][index];

                s->dsp.vector_fmul_add_add(ptr,window,output,ptr,0,2*n,1);

            }

        }

    }

 next:

    /* update block number */

    s->block_num++;

    s->block_pos += s->block_len;

    if (s->block_pos >= s->frame_len)

        return 1;

    else

        return 0;

}
