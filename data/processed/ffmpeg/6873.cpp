static void convert_matrix(DSPContext *dsp, int (*qmat)[64], uint16_t (*qmat16)[2][64],

                           const uint16_t *quant_matrix, int bias, int qmin, int qmax)

{

    int qscale;



    for(qscale=qmin; qscale<=qmax; qscale++){

        int i;

        if (dsp->fdct == ff_jpeg_fdct_islow 

#ifdef FAAN_POSTSCALE

            || dsp->fdct == ff_faandct

#endif

            ) {

            for(i=0;i<64;i++) {

                const int j= dsp->idct_permutation[i];

                /* 16 <= qscale * quant_matrix[i] <= 7905 */

                /* 19952         <= aanscales[i] * qscale * quant_matrix[i]           <= 249205026 */

                /* (1<<36)/19952 >= (1<<36)/(aanscales[i] * qscale * quant_matrix[i]) >= (1<<36)/249205026 */

                /* 3444240       >= (1<<36)/(aanscales[i] * qscale * quant_matrix[i]) >= 275 */

                

                qmat[qscale][i] = (int)((uint64_t_C(1) << QMAT_SHIFT) / 

                                (qscale * quant_matrix[j]));

            }

        } else if (dsp->fdct == fdct_ifast

#ifndef FAAN_POSTSCALE

                   || dsp->fdct == ff_faandct

#endif

                   ) {

            for(i=0;i<64;i++) {

                const int j= dsp->idct_permutation[i];

                /* 16 <= qscale * quant_matrix[i] <= 7905 */

                /* 19952         <= aanscales[i] * qscale * quant_matrix[i]           <= 249205026 */

                /* (1<<36)/19952 >= (1<<36)/(aanscales[i] * qscale * quant_matrix[i]) >= (1<<36)/249205026 */

                /* 3444240       >= (1<<36)/(aanscales[i] * qscale * quant_matrix[i]) >= 275 */

                

                qmat[qscale][i] = (int)((uint64_t_C(1) << (QMAT_SHIFT + 14)) / 

                                (aanscales[i] * qscale * quant_matrix[j]));

            }

        } else {

            for(i=0;i<64;i++) {

                const int j= dsp->idct_permutation[i];

                /* We can safely suppose that 16 <= quant_matrix[i] <= 255

                   So 16           <= qscale * quant_matrix[i]             <= 7905

                   so (1<<19) / 16 >= (1<<19) / (qscale * quant_matrix[i]) >= (1<<19) / 7905

                   so 32768        >= (1<<19) / (qscale * quant_matrix[i]) >= 67

                */

                qmat[qscale][i] = (int)((uint64_t_C(1) << QMAT_SHIFT) / (qscale * quant_matrix[j]));

//                qmat  [qscale][i] = (1 << QMAT_SHIFT_MMX) / (qscale * quant_matrix[i]);

                qmat16[qscale][0][i] = (1 << QMAT_SHIFT_MMX) / (qscale * quant_matrix[j]);



                if(qmat16[qscale][0][i]==0 || qmat16[qscale][0][i]==128*256) qmat16[qscale][0][i]=128*256-1;

                qmat16[qscale][1][i]= ROUNDED_DIV(bias<<(16-QUANT_BIAS_SHIFT), qmat16[qscale][0][i]);

            }

        }

    }

}
