void ff_put_h264_qpel8_mc12_msa(uint8_t *dst, const uint8_t *src,

                                ptrdiff_t stride)

{

    avc_luma_midh_qrt_8w_msa(src - (2 * stride) - 2, stride, dst, stride, 8, 0);

}
