void ff_dsputil_init_alpha(DSPContext* c, AVCodecContext *avctx)

{

    const int high_bit_depth = avctx->bits_per_raw_sample > 8;



    if (!high_bit_depth) {

    c->put_pixels_tab[0][0] = put_pixels16_axp_asm;

    c->put_pixels_tab[0][1] = put_pixels16_x2_axp;

    c->put_pixels_tab[0][2] = put_pixels16_y2_axp;

    c->put_pixels_tab[0][3] = put_pixels16_xy2_axp;



    c->put_no_rnd_pixels_tab[0][0] = put_pixels16_axp_asm;

    c->put_no_rnd_pixels_tab[0][1] = put_no_rnd_pixels16_x2_axp;

    c->put_no_rnd_pixels_tab[0][2] = put_no_rnd_pixels16_y2_axp;

    c->put_no_rnd_pixels_tab[0][3] = put_no_rnd_pixels16_xy2_axp;



    c->avg_pixels_tab[0][0] = avg_pixels16_axp;

    c->avg_pixels_tab[0][1] = avg_pixels16_x2_axp;

    c->avg_pixels_tab[0][2] = avg_pixels16_y2_axp;

    c->avg_pixels_tab[0][3] = avg_pixels16_xy2_axp;



    c->avg_no_rnd_pixels_tab[0][0] = avg_no_rnd_pixels16_axp;

    c->avg_no_rnd_pixels_tab[0][1] = avg_no_rnd_pixels16_x2_axp;

    c->avg_no_rnd_pixels_tab[0][2] = avg_no_rnd_pixels16_y2_axp;

    c->avg_no_rnd_pixels_tab[0][3] = avg_no_rnd_pixels16_xy2_axp;



    c->put_pixels_tab[1][0] = put_pixels_axp_asm;

    c->put_pixels_tab[1][1] = put_pixels_x2_axp;

    c->put_pixels_tab[1][2] = put_pixels_y2_axp;

    c->put_pixels_tab[1][3] = put_pixels_xy2_axp;



    c->put_no_rnd_pixels_tab[1][0] = put_pixels_axp_asm;

    c->put_no_rnd_pixels_tab[1][1] = put_no_rnd_pixels_x2_axp;

    c->put_no_rnd_pixels_tab[1][2] = put_no_rnd_pixels_y2_axp;

    c->put_no_rnd_pixels_tab[1][3] = put_no_rnd_pixels_xy2_axp;



    c->avg_pixels_tab[1][0] = avg_pixels_axp;

    c->avg_pixels_tab[1][1] = avg_pixels_x2_axp;

    c->avg_pixels_tab[1][2] = avg_pixels_y2_axp;

    c->avg_pixels_tab[1][3] = avg_pixels_xy2_axp;



    c->avg_no_rnd_pixels_tab[1][0] = avg_no_rnd_pixels_axp;

    c->avg_no_rnd_pixels_tab[1][1] = avg_no_rnd_pixels_x2_axp;

    c->avg_no_rnd_pixels_tab[1][2] = avg_no_rnd_pixels_y2_axp;

    c->avg_no_rnd_pixels_tab[1][3] = avg_no_rnd_pixels_xy2_axp;



    c->clear_blocks = clear_blocks_axp;

    }



    /* amask clears all bits that correspond to present features.  */

    if (amask(AMASK_MVI) == 0) {

        c->put_pixels_clamped = put_pixels_clamped_mvi_asm;

        c->add_pixels_clamped = add_pixels_clamped_mvi_asm;



        if (!high_bit_depth)

            c->get_pixels   = get_pixels_mvi;

        c->diff_pixels      = diff_pixels_mvi;

        c->sad[0]           = pix_abs16x16_mvi_asm;

        c->sad[1]           = pix_abs8x8_mvi;

        c->pix_abs[0][0]    = pix_abs16x16_mvi_asm;

        c->pix_abs[1][0]    = pix_abs8x8_mvi;

        c->pix_abs[0][1]    = pix_abs16x16_x2_mvi;

        c->pix_abs[0][2]    = pix_abs16x16_y2_mvi;

        c->pix_abs[0][3]    = pix_abs16x16_xy2_mvi;

    }



    put_pixels_clamped_axp_p = c->put_pixels_clamped;

    add_pixels_clamped_axp_p = c->add_pixels_clamped;



    if (avctx->bits_per_raw_sample <= 8 &&

        (avctx->idct_algo == FF_IDCT_AUTO ||

         avctx->idct_algo == FF_IDCT_SIMPLEALPHA)) {

        c->idct_put = ff_simple_idct_put_axp;

        c->idct_add = ff_simple_idct_add_axp;

        c->idct =     ff_simple_idct_axp;

    }

}
