static int decode_residual(H264Context *h, GetBitContext *gb, DCTELEM *block, int n, const uint8_t *scantable, const uint32_t *qmul, int max_coeff){

    MpegEncContext * const s = &h->s;

    static const int coeff_token_table_index[17]= {0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3};

    int level[16];

    int zeros_left, coeff_num, coeff_token, total_coeff, i, j, trailing_ones, run_before;



    //FIXME put trailing_onex into the context



    if(n == CHROMA_DC_BLOCK_INDEX){

        coeff_token= get_vlc2(gb, chroma_dc_coeff_token_vlc.table, CHROMA_DC_COEFF_TOKEN_VLC_BITS, 1);

        total_coeff= coeff_token>>2;

    }else{

        if(n == LUMA_DC_BLOCK_INDEX){

            total_coeff= pred_non_zero_count(h, 0);

            coeff_token= get_vlc2(gb, coeff_token_vlc[ coeff_token_table_index[total_coeff] ].table, COEFF_TOKEN_VLC_BITS, 2);

            total_coeff= coeff_token>>2;

        }else{

            total_coeff= pred_non_zero_count(h, n);

            coeff_token= get_vlc2(gb, coeff_token_vlc[ coeff_token_table_index[total_coeff] ].table, COEFF_TOKEN_VLC_BITS, 2);

            total_coeff= coeff_token>>2;

            h->non_zero_count_cache[ scan8[n] ]= total_coeff;

        }

    }



    //FIXME set last_non_zero?



    if(total_coeff==0)

        return 0;

    if(total_coeff > (unsigned)max_coeff) {

        av_log(h->s.avctx, AV_LOG_ERROR, "corrupted macroblock %d %d (total_coeff=%d)\n", s->mb_x, s->mb_y, total_coeff);

        return -1;

    }



    trailing_ones= coeff_token&3;

    tprintf(h->s.avctx, "trailing:%d, total:%d\n", trailing_ones, total_coeff);

    assert(total_coeff<=16);



    i = show_bits(gb, 3);

    skip_bits(gb, trailing_ones);

    level[0] = 1-((i&4)>>1);

    level[1] = 1-((i&2)   );

    level[2] = 1-((i&1)<<1);



    if(trailing_ones<total_coeff) {

        int mask, prefix;

        int suffix_length = total_coeff > 10 & trailing_ones < 3;

        int bitsi= show_bits(gb, LEVEL_TAB_BITS);

        int level_code= cavlc_level_tab[suffix_length][bitsi][0];



        skip_bits(gb, cavlc_level_tab[suffix_length][bitsi][1]);

        if(level_code >= 100){

            prefix= level_code - 100;

            if(prefix == LEVEL_TAB_BITS)

                prefix += get_level_prefix(gb);



            //first coefficient has suffix_length equal to 0 or 1

            if(prefix<14){ //FIXME try to build a large unified VLC table for all this

                if(suffix_length)

                    level_code= (prefix<<1) + get_bits1(gb); //part

                else

                    level_code= prefix; //part

            }else if(prefix==14){

                if(suffix_length)

                    level_code= (prefix<<1) + get_bits1(gb); //part

                else

                    level_code= prefix + get_bits(gb, 4); //part

            }else{

                level_code= 30 + get_bits(gb, prefix-3); //part

                if(prefix>=16)

                    level_code += (1<<(prefix-3))-4096;

            }



            if(trailing_ones < 3) level_code += 2;



            suffix_length = 2;

            mask= -(level_code&1);

            level[trailing_ones]= (((2+level_code)>>1) ^ mask) - mask;

        }else{

            level_code += ((level_code>>31)|1) & -(trailing_ones < 3);



            suffix_length = 1 + (level_code + 3U > 6U);

            level[trailing_ones]= level_code;

        }



        //remaining coefficients have suffix_length > 0

        for(i=trailing_ones+1;i<total_coeff;i++) {

            static const unsigned int suffix_limit[7] = {0,3,6,12,24,48,INT_MAX };

            int bitsi= show_bits(gb, LEVEL_TAB_BITS);

            level_code= cavlc_level_tab[suffix_length][bitsi][0];



            skip_bits(gb, cavlc_level_tab[suffix_length][bitsi][1]);

            if(level_code >= 100){

                prefix= level_code - 100;

                if(prefix == LEVEL_TAB_BITS){

                    prefix += get_level_prefix(gb);

                }

                if(prefix<15){

                    level_code = (prefix<<suffix_length) + get_bits(gb, suffix_length);

                }else{

                    level_code = (15<<suffix_length) + get_bits(gb, prefix-3);

                    if(prefix>=16)

                        level_code += (1<<(prefix-3))-4096;

                }

                mask= -(level_code&1);

                level_code= (((2+level_code)>>1) ^ mask) - mask;

            }

            level[i]= level_code;

            suffix_length+= suffix_limit[suffix_length] + level_code > 2U*suffix_limit[suffix_length];

        }

    }



    if(total_coeff == max_coeff)

        zeros_left=0;

    else{

        if(n == CHROMA_DC_BLOCK_INDEX)

            zeros_left= get_vlc2(gb, (chroma_dc_total_zeros_vlc-1)[ total_coeff ].table, CHROMA_DC_TOTAL_ZEROS_VLC_BITS, 1);

        else

            zeros_left= get_vlc2(gb, (total_zeros_vlc-1)[ total_coeff ].table, TOTAL_ZEROS_VLC_BITS, 1);

    }



    coeff_num = zeros_left + total_coeff - 1;

    j = scantable[coeff_num];

    if(n > 24){

        block[j] = level[0];

        for(i=1;i<total_coeff;i++) {

            if(zeros_left <= 0)

                run_before = 0;

            else if(zeros_left < 7){

                run_before= get_vlc2(gb, (run_vlc-1)[zeros_left].table, RUN_VLC_BITS, 1);

            }else{

                run_before= get_vlc2(gb, run7_vlc.table, RUN7_VLC_BITS, 2);

            }

            zeros_left -= run_before;

            coeff_num -= 1 + run_before;

            j= scantable[ coeff_num ];



            block[j]= level[i];

        }

    }else{

        block[j] = (level[0] * qmul[j] + 32)>>6;

        for(i=1;i<total_coeff;i++) {

            if(zeros_left <= 0)

                run_before = 0;

            else if(zeros_left < 7){

                run_before= get_vlc2(gb, (run_vlc-1)[zeros_left].table, RUN_VLC_BITS, 1);

            }else{

                run_before= get_vlc2(gb, run7_vlc.table, RUN7_VLC_BITS, 2);

            }

            zeros_left -= run_before;

            coeff_num -= 1 + run_before;

            j= scantable[ coeff_num ];



            block[j]= (level[i] * qmul[j] + 32)>>6;

        }

    }



    if(zeros_left<0){

        av_log(h->s.avctx, AV_LOG_ERROR, "negative number of zero coeffs at %d %d\n", s->mb_x, s->mb_y);

        return -1;

    }



    return 0;

}
