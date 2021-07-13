void ff_avg_h264_qpel16_mc22_msa(uint8_t *dst, const uint8_t *src,

                                 ptrdiff_t stride)

{

    avc_luma_mid_and_aver_dst_16x16_msa(src - (2 * stride) - 2,

                                        stride, dst, stride);

}
