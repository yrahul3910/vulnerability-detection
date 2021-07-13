static av_cold int MP3lame_encode_init(AVCodecContext *avctx)

{

    Mp3AudioContext *s = avctx->priv_data;



    if (avctx->channels > 2)

        return -1;



    s->stereo = avctx->channels > 1 ? 1 : 0;



    if ((s->gfp = lame_init()) == NULL)

        goto err;

    lame_set_in_samplerate(s->gfp, avctx->sample_rate);

    lame_set_out_samplerate(s->gfp, avctx->sample_rate);

    lame_set_num_channels(s->gfp, avctx->channels);

    if(avctx->compression_level == FF_COMPRESSION_DEFAULT) {

        lame_set_quality(s->gfp, 5);

    } else {

        lame_set_quality(s->gfp, avctx->compression_level);

    }

    lame_set_mode(s->gfp, s->stereo ? JOINT_STEREO : MONO);

    lame_set_brate(s->gfp, avctx->bit_rate/1000);

    if(avctx->flags & CODEC_FLAG_QSCALE) {

        lame_set_brate(s->gfp, 0);

        lame_set_VBR(s->gfp, vbr_default);

        lame_set_VBR_quality(s->gfp, avctx->global_quality/(float)FF_QP2LAMBDA);

    }

    lame_set_bWriteVbrTag(s->gfp,0);

    lame_set_disable_reservoir(s->gfp, avctx->flags2 & CODEC_FLAG2_BIT_RESERVOIR ? 0 : 1);

    if (lame_init_params(s->gfp) < 0)

        goto err_close;



    avctx->frame_size = lame_get_framesize(s->gfp);



    avctx->coded_frame= avcodec_alloc_frame();

    avctx->coded_frame->key_frame= 1;



    return 0;



err_close:

    lame_close(s->gfp);

err:

    return -1;

}
