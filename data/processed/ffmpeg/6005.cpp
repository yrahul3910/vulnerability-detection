static int decode_packet(AVCodecContext *avctx, void *data,
                         int *got_frame_ptr, AVPacket* avpkt)
{
    WMAProDecodeCtx *s = avctx->priv_data;
    GetBitContext* gb  = &s->pgb;
    const uint8_t* buf = avpkt->data;
    int buf_size       = avpkt->size;
    int num_bits_prev_frame;
    int packet_sequence_number;
    *got_frame_ptr = 0;
    if (s->skip_packets > 0) {
        s->skip_packets--;
        return FFMIN(avpkt->size, avctx->block_align);
    if (s->packet_done || s->packet_loss) {
        s->packet_done = 0;
        /** sanity check for the buffer length */
        if (avctx->codec_id == AV_CODEC_ID_WMAPRO && buf_size < avctx->block_align) {
            av_log(avctx, AV_LOG_ERROR, "Input packet too small (%d < %d)\n",
                   buf_size, avctx->block_align);
            return AVERROR_INVALIDDATA;
        if (avctx->codec_id == AV_CODEC_ID_WMAPRO) {
            s->next_packet_start = buf_size - avctx->block_align;
            buf_size = avctx->block_align;
        } else {
            s->next_packet_start = buf_size - FFMIN(buf_size, avctx->block_align);
            buf_size = FFMIN(buf_size, avctx->block_align);
        s->buf_bit_size = buf_size << 3;
        /** parse packet header */
        init_get_bits(gb, buf, s->buf_bit_size);
        if (avctx->codec_id != AV_CODEC_ID_XMA2) {
            packet_sequence_number = get_bits(gb, 4);
            skip_bits(gb, 2);
        } else {
            s->num_frames = get_bits(gb, 6);
            packet_sequence_number = 0;
        /** get number of bits that need to be added to the previous frame */
        num_bits_prev_frame = get_bits(gb, s->log2_frame_size);
        if (avctx->codec_id != AV_CODEC_ID_WMAPRO) {
            skip_bits(gb, 3);
            s->skip_packets = get_bits(gb, 8);
        ff_dlog(avctx, "packet[%d]: nbpf %x\n", avctx->frame_number,
                num_bits_prev_frame);
        /** check for packet loss */
        if (avctx->codec_id != AV_CODEC_ID_XMA2 && !s->packet_loss &&
            ((s->packet_sequence_number + 1) & 0xF) != packet_sequence_number) {
            av_log(avctx, AV_LOG_ERROR,
                   "Packet loss detected! seq %"PRIx8" vs %x\n",
                   s->packet_sequence_number, packet_sequence_number);
        s->packet_sequence_number = packet_sequence_number;
        if (num_bits_prev_frame > 0) {
            int remaining_packet_bits = s->buf_bit_size - get_bits_count(gb);
            if (num_bits_prev_frame >= remaining_packet_bits) {
                num_bits_prev_frame = remaining_packet_bits;
                s->packet_done = 1;
            /** append the previous frame data to the remaining data from the
                previous packet to create a full frame */
            save_bits(s, gb, num_bits_prev_frame, 1);
            ff_dlog(avctx, "accumulated %x bits of frame data\n",
                    s->num_saved_bits - s->frame_offset);
            /** decode the cross packet frame if it is valid */
            if (!s->packet_loss)
                decode_frame(s, data, got_frame_ptr);
        } else if (s->num_saved_bits - s->frame_offset) {
            ff_dlog(avctx, "ignoring %x previously saved bits\n",
                    s->num_saved_bits - s->frame_offset);
        if (s->packet_loss) {
            /** reset number of saved bits so that the decoder
                does not start to decode incomplete frames in the
                s->len_prefix == 0 case */
            s->num_saved_bits = 0;
            s->packet_loss = 0;
    } else {
        int frame_size;
        s->buf_bit_size = (avpkt->size - s->next_packet_start) << 3;
        init_get_bits(gb, avpkt->data, s->buf_bit_size);
        skip_bits(gb, s->packet_offset);
        if (s->len_prefix && remaining_bits(s, gb) > s->log2_frame_size &&
            (frame_size = show_bits(gb, s->log2_frame_size)) &&
            frame_size <= remaining_bits(s, gb)) {
            save_bits(s, gb, frame_size, 0);
            if (!s->packet_loss)
                s->packet_done = !decode_frame(s, data, got_frame_ptr);
        } else if (!s->len_prefix
                   && s->num_saved_bits > get_bits_count(&s->gb)) {
            /** when the frames do not have a length prefix, we don't know
                the compressed length of the individual frames
                however, we know what part of a new packet belongs to the
                previous frame
                therefore we save the incoming packet first, then we append
                the "previous frame" data from the next packet so that
                we get a buffer that only contains full frames */
            s->packet_done = !decode_frame(s, data, got_frame_ptr);
        } else
            s->packet_done = 1;
    if (s->packet_done && !s->packet_loss &&
        remaining_bits(s, gb) > 0) {
        /** save the rest of the data so that it can be decoded
            with the next packet */
        save_bits(s, gb, remaining_bits(s, gb), 0);
    s->packet_offset = get_bits_count(gb) & 7;
    if (s->packet_loss)
        return AVERROR_INVALIDDATA;
    return get_bits_count(gb) >> 3;