static int decode_pce(AVCodecContext *avctx, MPEG4AudioConfig *m4ac,

                      enum ChannelPosition new_che_pos[4][MAX_ELEM_ID],

                      GetBitContext *gb)

{

    int num_front, num_side, num_back, num_lfe, num_assoc_data, num_cc, sampling_index;

    int comment_len;



    skip_bits(gb, 2);  // object_type



    sampling_index = get_bits(gb, 4);

    if (m4ac->sampling_index != sampling_index)

        av_log(avctx, AV_LOG_WARNING, "Sample rate index in program config element does not match the sample rate index configured by the container.\n");



    num_front       = get_bits(gb, 4);

    num_side        = get_bits(gb, 4);

    num_back        = get_bits(gb, 4);

    num_lfe         = get_bits(gb, 2);

    num_assoc_data  = get_bits(gb, 3);

    num_cc          = get_bits(gb, 4);



    if (get_bits1(gb))

        skip_bits(gb, 4); // mono_mixdown_tag

    if (get_bits1(gb))

        skip_bits(gb, 4); // stereo_mixdown_tag



    if (get_bits1(gb))

        skip_bits(gb, 3); // mixdown_coeff_index and pseudo_surround







    decode_channel_map(new_che_pos[TYPE_CPE], new_che_pos[TYPE_SCE], AAC_CHANNEL_FRONT, gb, num_front);

    decode_channel_map(new_che_pos[TYPE_CPE], new_che_pos[TYPE_SCE], AAC_CHANNEL_SIDE,  gb, num_side );

    decode_channel_map(new_che_pos[TYPE_CPE], new_che_pos[TYPE_SCE], AAC_CHANNEL_BACK,  gb, num_back );

    decode_channel_map(NULL,                  new_che_pos[TYPE_LFE], AAC_CHANNEL_LFE,   gb, num_lfe  );



    skip_bits_long(gb, 4 * num_assoc_data);



    decode_channel_map(new_che_pos[TYPE_CCE], new_che_pos[TYPE_CCE], AAC_CHANNEL_CC,    gb, num_cc   );



    align_get_bits(gb);



    /* comment field, first byte is length */

    comment_len = get_bits(gb, 8) * 8;

    if (get_bits_left(gb) < comment_len) {




    skip_bits_long(gb, comment_len);

    return 0;
