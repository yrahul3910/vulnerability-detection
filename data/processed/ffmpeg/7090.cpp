void ff_h264_pred_init_x86(H264PredContext *h, int codec_id, const int bit_depth, const int chroma_format_idc)

{

#if HAVE_YASM

    int mm_flags = av_get_cpu_flags();



    if (bit_depth == 8) {

        if (mm_flags & AV_CPU_FLAG_MMX) {

            h->pred16x16[VERT_PRED8x8         ] = ff_pred16x16_vertical_mmx;

            h->pred16x16[HOR_PRED8x8          ] = ff_pred16x16_horizontal_mmx;

            if (chroma_format_idc == 1) {

                h->pred8x8  [VERT_PRED8x8     ] = ff_pred8x8_vertical_mmx;

                h->pred8x8  [HOR_PRED8x8      ] = ff_pred8x8_horizontal_mmx;

            }

            if (codec_id == AV_CODEC_ID_VP8) {

                h->pred16x16[PLANE_PRED8x8    ] = ff_pred16x16_tm_vp8_mmx;

                h->pred8x8  [PLANE_PRED8x8    ] = ff_pred8x8_tm_vp8_mmx;

                h->pred4x4  [TM_VP8_PRED      ] = ff_pred4x4_tm_vp8_mmx;

            } else {

                if (chroma_format_idc == 1)

                    h->pred8x8  [PLANE_PRED8x8] = ff_pred8x8_plane_mmx;

                if (codec_id == AV_CODEC_ID_SVQ3) {

                    if (mm_flags & AV_CPU_FLAG_CMOV)

                        h->pred16x16[PLANE_PRED8x8] = ff_pred16x16_plane_svq3_mmx;

                } else if (codec_id == AV_CODEC_ID_RV40) {

                    h->pred16x16[PLANE_PRED8x8] = ff_pred16x16_plane_rv40_mmx;

                } else {

                    h->pred16x16[PLANE_PRED8x8] = ff_pred16x16_plane_h264_mmx;

                }

            }

        }



        if (mm_flags & AV_CPU_FLAG_MMXEXT) {

            h->pred16x16[HOR_PRED8x8            ] = ff_pred16x16_horizontal_mmx2;

            h->pred16x16[DC_PRED8x8             ] = ff_pred16x16_dc_mmx2;

            if (chroma_format_idc == 1)

                h->pred8x8[HOR_PRED8x8          ] = ff_pred8x8_horizontal_mmx2;

            h->pred8x8l [TOP_DC_PRED            ] = ff_pred8x8l_top_dc_mmxext;

            h->pred8x8l [DC_PRED                ] = ff_pred8x8l_dc_mmxext;

            h->pred8x8l [HOR_PRED               ] = ff_pred8x8l_horizontal_mmxext;

            h->pred8x8l [VERT_PRED              ] = ff_pred8x8l_vertical_mmxext;

            h->pred8x8l [DIAG_DOWN_RIGHT_PRED   ] = ff_pred8x8l_down_right_mmxext;

            h->pred8x8l [VERT_RIGHT_PRED        ] = ff_pred8x8l_vertical_right_mmxext;

            h->pred8x8l [HOR_UP_PRED            ] = ff_pred8x8l_horizontal_up_mmxext;

            h->pred8x8l [DIAG_DOWN_LEFT_PRED    ] = ff_pred8x8l_down_left_mmxext;

            h->pred8x8l [HOR_DOWN_PRED          ] = ff_pred8x8l_horizontal_down_mmxext;

            h->pred4x4  [DIAG_DOWN_RIGHT_PRED   ] = ff_pred4x4_down_right_mmxext;

            h->pred4x4  [VERT_RIGHT_PRED        ] = ff_pred4x4_vertical_right_mmxext;

            h->pred4x4  [HOR_DOWN_PRED          ] = ff_pred4x4_horizontal_down_mmxext;

            h->pred4x4  [DC_PRED                ] = ff_pred4x4_dc_mmxext;

            if (codec_id == AV_CODEC_ID_VP8 || codec_id == AV_CODEC_ID_H264) {

                h->pred4x4  [DIAG_DOWN_LEFT_PRED] = ff_pred4x4_down_left_mmxext;

            }

            if (codec_id == AV_CODEC_ID_SVQ3 || codec_id == AV_CODEC_ID_H264) {

                h->pred4x4  [VERT_LEFT_PRED     ] = ff_pred4x4_vertical_left_mmxext;

            }

            if (codec_id != AV_CODEC_ID_RV40) {

                h->pred4x4  [HOR_UP_PRED        ] = ff_pred4x4_horizontal_up_mmxext;

            }

            if (codec_id == AV_CODEC_ID_SVQ3 || codec_id == AV_CODEC_ID_H264) {

                if (chroma_format_idc == 1) {

                    h->pred8x8[TOP_DC_PRED8x8   ] = ff_pred8x8_top_dc_mmxext;

                    h->pred8x8[DC_PRED8x8       ] = ff_pred8x8_dc_mmxext;

                }

            }

            if (codec_id == AV_CODEC_ID_VP8) {

                h->pred16x16[PLANE_PRED8x8      ] = ff_pred16x16_tm_vp8_mmx2;

                h->pred8x8  [DC_PRED8x8         ] = ff_pred8x8_dc_rv40_mmxext;

                h->pred8x8  [PLANE_PRED8x8      ] = ff_pred8x8_tm_vp8_mmx2;

                h->pred4x4  [TM_VP8_PRED        ] = ff_pred4x4_tm_vp8_mmx2;

                h->pred4x4  [VERT_PRED          ] = ff_pred4x4_vertical_vp8_mmxext;

            } else {

                if (chroma_format_idc == 1)

                    h->pred8x8  [PLANE_PRED8x8] = ff_pred8x8_plane_mmx2;

                if (codec_id == AV_CODEC_ID_SVQ3) {

                    h->pred16x16[PLANE_PRED8x8  ] = ff_pred16x16_plane_svq3_mmx2;

                } else if (codec_id == AV_CODEC_ID_RV40) {

                    h->pred16x16[PLANE_PRED8x8  ] = ff_pred16x16_plane_rv40_mmx2;

                } else {

                    h->pred16x16[PLANE_PRED8x8  ] = ff_pred16x16_plane_h264_mmx2;

                }

            }

        }



        if (mm_flags & AV_CPU_FLAG_SSE) {

            h->pred16x16[VERT_PRED8x8] = ff_pred16x16_vertical_sse;

        }



        if (mm_flags & AV_CPU_FLAG_SSE2) {

            h->pred16x16[DC_PRED8x8           ] = ff_pred16x16_dc_sse2;

            h->pred8x8l [DIAG_DOWN_LEFT_PRED  ] = ff_pred8x8l_down_left_sse2;

            h->pred8x8l [DIAG_DOWN_RIGHT_PRED ] = ff_pred8x8l_down_right_sse2;

            h->pred8x8l [VERT_RIGHT_PRED      ] = ff_pred8x8l_vertical_right_sse2;

            h->pred8x8l [VERT_LEFT_PRED       ] = ff_pred8x8l_vertical_left_sse2;

            h->pred8x8l [HOR_DOWN_PRED        ] = ff_pred8x8l_horizontal_down_sse2;

            if (codec_id == AV_CODEC_ID_VP8) {

                h->pred16x16[PLANE_PRED8x8    ] = ff_pred16x16_tm_vp8_sse2;

                h->pred8x8  [PLANE_PRED8x8    ] = ff_pred8x8_tm_vp8_sse2;

            } else {

                if (chroma_format_idc == 1)

                    h->pred8x8  [PLANE_PRED8x8] = ff_pred8x8_plane_sse2;

                if (codec_id == AV_CODEC_ID_SVQ3) {

                    h->pred16x16[PLANE_PRED8x8] = ff_pred16x16_plane_svq3_sse2;

                } else if (codec_id == AV_CODEC_ID_RV40) {

                    h->pred16x16[PLANE_PRED8x8] = ff_pred16x16_plane_rv40_sse2;

                } else {

                    h->pred16x16[PLANE_PRED8x8] = ff_pred16x16_plane_h264_sse2;

                }

            }

        }



        if (mm_flags & AV_CPU_FLAG_SSSE3) {

            h->pred16x16[HOR_PRED8x8          ] = ff_pred16x16_horizontal_ssse3;

            h->pred16x16[DC_PRED8x8           ] = ff_pred16x16_dc_ssse3;

            if (chroma_format_idc == 1)

                h->pred8x8  [HOR_PRED8x8      ] = ff_pred8x8_horizontal_ssse3;

            h->pred8x8l [TOP_DC_PRED          ] = ff_pred8x8l_top_dc_ssse3;

            h->pred8x8l [DC_PRED              ] = ff_pred8x8l_dc_ssse3;

            h->pred8x8l [HOR_PRED             ] = ff_pred8x8l_horizontal_ssse3;

            h->pred8x8l [VERT_PRED            ] = ff_pred8x8l_vertical_ssse3;

            h->pred8x8l [DIAG_DOWN_LEFT_PRED  ] = ff_pred8x8l_down_left_ssse3;

            h->pred8x8l [DIAG_DOWN_RIGHT_PRED ] = ff_pred8x8l_down_right_ssse3;

            h->pred8x8l [VERT_RIGHT_PRED      ] = ff_pred8x8l_vertical_right_ssse3;

            h->pred8x8l [VERT_LEFT_PRED       ] = ff_pred8x8l_vertical_left_ssse3;

            h->pred8x8l [HOR_UP_PRED          ] = ff_pred8x8l_horizontal_up_ssse3;

            h->pred8x8l [HOR_DOWN_PRED        ] = ff_pred8x8l_horizontal_down_ssse3;

            if (codec_id == AV_CODEC_ID_VP8) {

                h->pred8x8  [PLANE_PRED8x8    ] = ff_pred8x8_tm_vp8_ssse3;

                h->pred4x4  [TM_VP8_PRED      ] = ff_pred4x4_tm_vp8_ssse3;

            } else {

                if (chroma_format_idc == 1)

                    h->pred8x8  [PLANE_PRED8x8] = ff_pred8x8_plane_ssse3;

                if (codec_id == AV_CODEC_ID_SVQ3) {

                    h->pred16x16[PLANE_PRED8x8] = ff_pred16x16_plane_svq3_ssse3;

                } else if (codec_id == AV_CODEC_ID_RV40) {

                    h->pred16x16[PLANE_PRED8x8] = ff_pred16x16_plane_rv40_ssse3;

                } else {

                    h->pred16x16[PLANE_PRED8x8] = ff_pred16x16_plane_h264_ssse3;

                }

            }

        }

    } else if (bit_depth == 10) {

        if (mm_flags & AV_CPU_FLAG_MMXEXT) {

            h->pred4x4[DC_PRED             ] = ff_pred4x4_dc_10_mmxext;

            h->pred4x4[HOR_UP_PRED         ] = ff_pred4x4_horizontal_up_10_mmxext;



            if (chroma_format_idc == 1)

                h->pred8x8[DC_PRED8x8      ] = ff_pred8x8_dc_10_mmxext;



            h->pred8x8l[DC_128_PRED        ] = ff_pred8x8l_128_dc_10_mmxext;



            h->pred16x16[DC_PRED8x8        ] = ff_pred16x16_dc_10_mmxext;

            h->pred16x16[TOP_DC_PRED8x8    ] = ff_pred16x16_top_dc_10_mmxext;

            h->pred16x16[DC_128_PRED8x8    ] = ff_pred16x16_128_dc_10_mmxext;

            h->pred16x16[LEFT_DC_PRED8x8   ] = ff_pred16x16_left_dc_10_mmxext;

            h->pred16x16[VERT_PRED8x8      ] = ff_pred16x16_vertical_10_mmxext;

            h->pred16x16[HOR_PRED8x8       ] = ff_pred16x16_horizontal_10_mmxext;

        }

        if (mm_flags & AV_CPU_FLAG_SSE2) {

            h->pred4x4[DIAG_DOWN_LEFT_PRED ] = ff_pred4x4_down_left_10_sse2;

            h->pred4x4[DIAG_DOWN_RIGHT_PRED] = ff_pred4x4_down_right_10_sse2;

            h->pred4x4[VERT_LEFT_PRED      ] = ff_pred4x4_vertical_left_10_sse2;

            h->pred4x4[VERT_RIGHT_PRED     ] = ff_pred4x4_vertical_right_10_sse2;

            h->pred4x4[HOR_DOWN_PRED       ] = ff_pred4x4_horizontal_down_10_sse2;



            if (chroma_format_idc == 1) {

                h->pred8x8[DC_PRED8x8      ] = ff_pred8x8_dc_10_sse2;

                h->pred8x8[TOP_DC_PRED8x8  ] = ff_pred8x8_top_dc_10_sse2;

                h->pred8x8[PLANE_PRED8x8   ] = ff_pred8x8_plane_10_sse2;

                h->pred8x8[VERT_PRED8x8    ] = ff_pred8x8_vertical_10_sse2;

                h->pred8x8[HOR_PRED8x8     ] = ff_pred8x8_horizontal_10_sse2;

            }



            h->pred8x8l[VERT_PRED           ] = ff_pred8x8l_vertical_10_sse2;

            h->pred8x8l[HOR_PRED            ] = ff_pred8x8l_horizontal_10_sse2;

            h->pred8x8l[DC_PRED             ] = ff_pred8x8l_dc_10_sse2;

            h->pred8x8l[DC_128_PRED         ] = ff_pred8x8l_128_dc_10_sse2;

            h->pred8x8l[TOP_DC_PRED         ] = ff_pred8x8l_top_dc_10_sse2;

            h->pred8x8l[DIAG_DOWN_LEFT_PRED ] = ff_pred8x8l_down_left_10_sse2;

            h->pred8x8l[DIAG_DOWN_RIGHT_PRED] = ff_pred8x8l_down_right_10_sse2;

            h->pred8x8l[VERT_RIGHT_PRED     ] = ff_pred8x8l_vertical_right_10_sse2;

            h->pred8x8l[HOR_UP_PRED         ] = ff_pred8x8l_horizontal_up_10_sse2;



            h->pred16x16[DC_PRED8x8        ] = ff_pred16x16_dc_10_sse2;

            h->pred16x16[TOP_DC_PRED8x8    ] = ff_pred16x16_top_dc_10_sse2;

            h->pred16x16[DC_128_PRED8x8    ] = ff_pred16x16_128_dc_10_sse2;

            h->pred16x16[LEFT_DC_PRED8x8   ] = ff_pred16x16_left_dc_10_sse2;

            h->pred16x16[VERT_PRED8x8      ] = ff_pred16x16_vertical_10_sse2;

            h->pred16x16[HOR_PRED8x8       ] = ff_pred16x16_horizontal_10_sse2;

        }

        if (mm_flags & AV_CPU_FLAG_SSSE3) {

            h->pred4x4[DIAG_DOWN_RIGHT_PRED] = ff_pred4x4_down_right_10_ssse3;

            h->pred4x4[VERT_RIGHT_PRED     ] = ff_pred4x4_vertical_right_10_ssse3;

            h->pred4x4[HOR_DOWN_PRED       ] = ff_pred4x4_horizontal_down_10_ssse3;



            h->pred8x8l[HOR_PRED            ] = ff_pred8x8l_horizontal_10_ssse3;

            h->pred8x8l[DIAG_DOWN_LEFT_PRED ] = ff_pred8x8l_down_left_10_ssse3;

            h->pred8x8l[DIAG_DOWN_RIGHT_PRED] = ff_pred8x8l_down_right_10_ssse3;

            h->pred8x8l[VERT_RIGHT_PRED     ] = ff_pred8x8l_vertical_right_10_ssse3;

            h->pred8x8l[HOR_UP_PRED         ] = ff_pred8x8l_horizontal_up_10_ssse3;

        }

#if HAVE_AVX

        if (mm_flags & AV_CPU_FLAG_AVX) {

            h->pred4x4[DIAG_DOWN_LEFT_PRED ] = ff_pred4x4_down_left_10_avx;

            h->pred4x4[DIAG_DOWN_RIGHT_PRED] = ff_pred4x4_down_right_10_avx;

            h->pred4x4[VERT_LEFT_PRED      ] = ff_pred4x4_vertical_left_10_avx;

            h->pred4x4[VERT_RIGHT_PRED     ] = ff_pred4x4_vertical_right_10_avx;

            h->pred4x4[HOR_DOWN_PRED       ] = ff_pred4x4_horizontal_down_10_avx;



            h->pred8x8l[VERT_PRED           ] = ff_pred8x8l_vertical_10_avx;

            h->pred8x8l[HOR_PRED            ] = ff_pred8x8l_horizontal_10_avx;

            h->pred8x8l[DC_PRED             ] = ff_pred8x8l_dc_10_avx;

            h->pred8x8l[TOP_DC_PRED         ] = ff_pred8x8l_top_dc_10_avx;

            h->pred8x8l[DIAG_DOWN_RIGHT_PRED] = ff_pred8x8l_down_right_10_avx;

            h->pred8x8l[DIAG_DOWN_LEFT_PRED ] = ff_pred8x8l_down_left_10_avx;

            h->pred8x8l[VERT_RIGHT_PRED     ] = ff_pred8x8l_vertical_right_10_avx;

            h->pred8x8l[HOR_UP_PRED         ] = ff_pred8x8l_horizontal_up_10_avx;

        }

#endif /* HAVE_AVX */

    }

#endif /* HAVE_YASM */

}
