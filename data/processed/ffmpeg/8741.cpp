void ff_dsputil_init_ppc(DSPContext* c, AVCodecContext *avctx)

{

    const int high_bit_depth = avctx->bits_per_raw_sample > 8;



    // Common optimizations whether AltiVec is available or not

    c->prefetch = prefetch_ppc;

    if (!high_bit_depth) {

    switch (check_dcbzl_effect()) {

        case 32:

            c->clear_blocks = clear_blocks_dcbz32_ppc;

            break;

        case 128:

            c->clear_blocks = clear_blocks_dcbz128_ppc;

            break;

        default:

            break;

    }

    }



#if HAVE_ALTIVEC

    if(CONFIG_H264_DECODER) ff_dsputil_h264_init_ppc(c, avctx);



    if (av_get_cpu_flags() & AV_CPU_FLAG_ALTIVEC) {

        ff_dsputil_init_altivec(c, avctx);

        ff_float_init_altivec(c, avctx);

        ff_int_init_altivec(c, avctx);

        c->gmc1 = ff_gmc1_altivec;



#if CONFIG_ENCODERS

        if (avctx->bits_per_raw_sample <= 8 &&

            (avctx->dct_algo == FF_DCT_AUTO ||

             avctx->dct_algo == FF_DCT_ALTIVEC)) {

            c->fdct = ff_fdct_altivec;

        }

#endif //CONFIG_ENCODERS



        if (avctx->bits_per_raw_sample <= 8) {

            if ((avctx->idct_algo == FF_IDCT_AUTO) ||

                (avctx->idct_algo == FF_IDCT_ALTIVEC)) {

                c->idct_put = ff_idct_put_altivec;

                c->idct_add = ff_idct_add_altivec;

                c->idct_permutation_type = FF_TRANSPOSE_IDCT_PERM;

            }else if((CONFIG_VP3_DECODER || CONFIG_VP5_DECODER || CONFIG_VP6_DECODER) &&

                     avctx->idct_algo==FF_IDCT_VP3){

                c->idct_put = ff_vp3_idct_put_altivec;

                c->idct_add = ff_vp3_idct_add_altivec;

                c->idct     = ff_vp3_idct_altivec;

                c->idct_permutation_type = FF_TRANSPOSE_IDCT_PERM;

            }

        }



    }

#endif /* HAVE_ALTIVEC */

}
