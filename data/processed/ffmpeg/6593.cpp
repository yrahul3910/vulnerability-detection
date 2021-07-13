int ff_rate_control_init(MpegEncContext *s)

{

    RateControlContext *rcc= &s->rc_context;

    int i;

    char *error = NULL;

    static const char *const_names[]={

        "PI",

        "E",

        "iTex",

        "pTex",

        "tex",

        "mv",

        "fCode",

        "iCount",

        "mcVar",

        "var",

        "isI",

        "isP",

        "isB",

        "avgQP",

        "qComp",

/*        "lastIQP",

        "lastPQP",

        "lastBQP",

        "nextNonBQP",*/

        "avgIITex",

        "avgPITex",

        "avgPPTex",

        "avgBPTex",

        "avgTex",

        NULL

    };

    static double (*func1[])(void *, double)={

        (void *)bits2qp,

        (void *)qp2bits,

        NULL

    };

    static const char *func1_names[]={

        "bits2qp",

        "qp2bits",

        NULL

    };

    emms_c();



    rcc->rc_eq_eval = ff_parse(s->avctx->rc_eq, const_names, func1, func1_names, NULL, NULL, &error);

    if (!rcc->rc_eq_eval) {

        av_log(s->avctx, AV_LOG_ERROR, "Error parsing rc_eq \"%s\": %s\n", s->avctx->rc_eq, error? error : "");

        return -1;

    }



    for(i=0; i<5; i++){

        rcc->pred[i].coeff= FF_QP2LAMBDA * 7.0;

        rcc->pred[i].count= 1.0;



        rcc->pred[i].decay= 0.4;

        rcc->i_cplx_sum [i]=

        rcc->p_cplx_sum [i]=

        rcc->mv_bits_sum[i]=

        rcc->qscale_sum [i]=

        rcc->frame_count[i]= 1; // 1 is better cuz of 1/0 and such

        rcc->last_qscale_for[i]=FF_QP2LAMBDA * 5;

    }

    rcc->buffer_index= s->avctx->rc_initial_buffer_occupancy;



    if(s->flags&CODEC_FLAG_PASS2){

        int i;

        char *p;



        /* find number of pics */

        p= s->avctx->stats_in;

        for(i=-1; p; i++){

            p= strchr(p+1, ';');

        }

        i+= s->max_b_frames;

        if(i<=0 || i>=INT_MAX / sizeof(RateControlEntry))

            return -1;

        rcc->entry = (RateControlEntry*)av_mallocz(i*sizeof(RateControlEntry));

        rcc->num_entries= i;



        /* init all to skipped p frames (with b frames we might have a not encoded frame at the end FIXME) */

        for(i=0; i<rcc->num_entries; i++){

            RateControlEntry *rce= &rcc->entry[i];

            rce->pict_type= rce->new_pict_type=P_TYPE;

            rce->qscale= rce->new_qscale=FF_QP2LAMBDA * 2;

            rce->misc_bits= s->mb_num + 10;

            rce->mb_var_sum= s->mb_num*100;

        }



        /* read stats */

        p= s->avctx->stats_in;

        for(i=0; i<rcc->num_entries - s->max_b_frames; i++){

            RateControlEntry *rce;

            int picture_number;

            int e;

            char *next;



            next= strchr(p, ';');

            if(next){

                (*next)=0; //sscanf in unbelievably slow on looong strings //FIXME copy / do not write

                next++;

            }

            e= sscanf(p, " in:%d ", &picture_number);



            assert(picture_number >= 0);

            assert(picture_number < rcc->num_entries);

            rce= &rcc->entry[picture_number];



            e+=sscanf(p, " in:%*d out:%*d type:%d q:%f itex:%d ptex:%d mv:%d misc:%d fcode:%d bcode:%d mc-var:%d var:%d icount:%d skipcount:%d hbits:%d",

                   &rce->pict_type, &rce->qscale, &rce->i_tex_bits, &rce->p_tex_bits, &rce->mv_bits, &rce->misc_bits,

                   &rce->f_code, &rce->b_code, &rce->mc_mb_var_sum, &rce->mb_var_sum, &rce->i_count, &rce->skip_count, &rce->header_bits);

            if(e!=14){

                av_log(s->avctx, AV_LOG_ERROR, "statistics are damaged at line %d, parser out=%d\n", i, e);

                return -1;

            }



            p= next;

        }



        if(init_pass2(s) < 0) return -1;



        //FIXME maybe move to end

        if((s->flags&CODEC_FLAG_PASS2) && s->avctx->rc_strategy == FF_RC_STRATEGY_XVID) {

#ifdef CONFIG_LIBXVID

            return ff_xvid_rate_control_init(s);

#else

            av_log(s->avctx, AV_LOG_ERROR, "XviD ratecontrol requires libavcodec compiled with XviD support\n");

            return -1;

#endif

        }

    }



    if(!(s->flags&CODEC_FLAG_PASS2)){



        rcc->short_term_qsum=0.001;

        rcc->short_term_qcount=0.001;



        rcc->pass1_rc_eq_output_sum= 0.001;

        rcc->pass1_wanted_bits=0.001;



        if(s->avctx->qblur > 1.0){

            av_log(s->avctx, AV_LOG_ERROR, "qblur too large\n");

            return -1;

        }

        /* init stuff with the user specified complexity */

        if(s->avctx->rc_initial_cplx){

            for(i=0; i<60*30; i++){

                double bits= s->avctx->rc_initial_cplx * (i/10000.0 + 1.0)*s->mb_num;

                RateControlEntry rce;

                double q;



                if     (i%((s->gop_size+3)/4)==0) rce.pict_type= I_TYPE;

                else if(i%(s->max_b_frames+1))    rce.pict_type= B_TYPE;

                else                              rce.pict_type= P_TYPE;



                rce.new_pict_type= rce.pict_type;

                rce.mc_mb_var_sum= bits*s->mb_num/100000;

                rce.mb_var_sum   = s->mb_num;

                rce.qscale   = FF_QP2LAMBDA * 2;

                rce.f_code   = 2;

                rce.b_code   = 1;

                rce.misc_bits= 1;



                if(s->pict_type== I_TYPE){

                    rce.i_count   = s->mb_num;

                    rce.i_tex_bits= bits;

                    rce.p_tex_bits= 0;

                    rce.mv_bits= 0;

                }else{

                    rce.i_count   = 0; //FIXME we do know this approx

                    rce.i_tex_bits= 0;

                    rce.p_tex_bits= bits*0.9;

                    rce.mv_bits= bits*0.1;

                }

                rcc->i_cplx_sum [rce.pict_type] += rce.i_tex_bits*rce.qscale;

                rcc->p_cplx_sum [rce.pict_type] += rce.p_tex_bits*rce.qscale;

                rcc->mv_bits_sum[rce.pict_type] += rce.mv_bits;

                rcc->frame_count[rce.pict_type] ++;



                bits= rce.i_tex_bits + rce.p_tex_bits;



                q= get_qscale(s, &rce, rcc->pass1_wanted_bits/rcc->pass1_rc_eq_output_sum, i);

                rcc->pass1_wanted_bits+= s->bit_rate/(1/av_q2d(s->avctx->time_base)); //FIXME misbehaves a little for variable fps

            }

        }



    }



    return 0;

}
