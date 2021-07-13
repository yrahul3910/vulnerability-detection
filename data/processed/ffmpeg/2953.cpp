int swr_convert(struct SwrContext *s, uint8_t *out_arg[SWR_CH_MAX], int out_count,

                                const uint8_t *in_arg [SWR_CH_MAX], int  in_count){

    AudioData * in= &s->in;

    AudioData *out= &s->out;



    if(s->drop_output > 0){

        int ret;

        uint8_t *tmp_arg[SWR_CH_MAX];

        if((ret=swri_realloc_audio(&s->drop_temp, s->drop_output))<0)

            return ret;



        reversefill_audiodata(&s->drop_temp, tmp_arg);

        s->drop_output *= -1; //FIXME find a less hackish solution

        ret = swr_convert(s, tmp_arg, -s->drop_output, in_arg, in_count); //FIXME optimize but this is as good as never called so maybe it doesnt matter

        s->drop_output *= -1;

        if(ret>0)

            s->drop_output -= ret;



        if(s->drop_output || !out_arg)

            return 0;

        in_count = 0;

    }



    if(!in_arg){

        if(s->resample){

            if (!s->flushed)

                s->resampler->flush(s);

            s->resample_in_constraint = 0;

            s->flushed = 1;

        }else if(!s->in_buffer_count){

            return 0;

        }

    }else

        fill_audiodata(in ,  (void*)in_arg);



    fill_audiodata(out, out_arg);



    if(s->resample){

        int ret = swr_convert_internal(s, out, out_count, in, in_count);

        if(ret>0 && !s->drop_output)

            s->outpts += ret * (int64_t)s->in_sample_rate;

        return ret;

    }else{

        AudioData tmp= *in;

        int ret2=0;

        int ret, size;

        size = FFMIN(out_count, s->in_buffer_count);

        if(size){

            buf_set(&tmp, &s->in_buffer, s->in_buffer_index);

            ret= swr_convert_internal(s, out, size, &tmp, size);

            if(ret<0)

                return ret;

            ret2= ret;

            s->in_buffer_count -= ret;

            s->in_buffer_index += ret;

            buf_set(out, out, ret);

            out_count -= ret;

            if(!s->in_buffer_count)

                s->in_buffer_index = 0;

        }



        if(in_count){

            size= s->in_buffer_index + s->in_buffer_count + in_count - out_count;



            if(in_count > out_count) { //FIXME move after swr_convert_internal

                if(   size > s->in_buffer.count

                && s->in_buffer_count + in_count - out_count <= s->in_buffer_index){

                    buf_set(&tmp, &s->in_buffer, s->in_buffer_index);

                    copy(&s->in_buffer, &tmp, s->in_buffer_count);

                    s->in_buffer_index=0;

                }else

                    if((ret=swri_realloc_audio(&s->in_buffer, size)) < 0)

                        return ret;

            }



            if(out_count){

                size = FFMIN(in_count, out_count);

                ret= swr_convert_internal(s, out, size, in, size);

                if(ret<0)

                    return ret;

                buf_set(in, in, ret);

                in_count -= ret;

                ret2 += ret;

            }

            if(in_count){

                buf_set(&tmp, &s->in_buffer, s->in_buffer_index + s->in_buffer_count);

                copy(&tmp, in, in_count);

                s->in_buffer_count += in_count;

            }

        }

        if(ret2>0 && !s->drop_output)

            s->outpts += ret2 * (int64_t)s->in_sample_rate;

        return ret2;

    }

}
