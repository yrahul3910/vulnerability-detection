void ff_put_h264_qpel4_mc32_msa(uint8_t *dst, const uint8_t *src,

                                ptrdiff_t stride)

{

    avc_luma_midh_qrt_4w_msa(src - (2 * stride) - 2, stride, dst, stride, 4, 1);

}
