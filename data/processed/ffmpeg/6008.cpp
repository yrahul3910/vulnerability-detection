static void mpeg1_encode_sequence_header(MpegEncContext *s)

{

        unsigned int vbv_buffer_size;

        unsigned int fps, v;

        int n;

        UINT64 time_code;

        

        if ((s->picture_number % s->gop_size) == 0) {

            /* mpeg1 header repeated every gop */

            put_header(s, SEQ_START_CODE);

            

            /* search closest frame rate */

            {

                int i, dmin, d;

                s->frame_rate_index = 0;

                dmin = 0x7fffffff;

                for(i=1;i<9;i++) {

                    d = abs(s->frame_rate - frame_rate_tab[i]);

                    if (d < dmin) {

                        dmin = d;

                        s->frame_rate_index = i;

                    }

                }

            }

 

            put_bits(&s->pb, 12, s->width);

            put_bits(&s->pb, 12, s->height);

            put_bits(&s->pb, 4, 1); /* 1/1 aspect ratio */

            put_bits(&s->pb, 4, s->frame_rate_index);

            v = s->bit_rate / 400;

            if (v > 0x3ffff)

                v = 0x3ffff;

            put_bits(&s->pb, 18, v);

            put_bits(&s->pb, 1, 1); /* marker */

            /* vbv buffer size: slightly greater than an I frame. We add

               some margin just in case */

            vbv_buffer_size = (3 * s->I_frame_bits) / (2 * 8);

            put_bits(&s->pb, 10, (vbv_buffer_size + 16383) / 16384); 

            put_bits(&s->pb, 1, 1); /* constrained parameter flag */

            put_bits(&s->pb, 1, 0); /* no custom intra matrix */

            put_bits(&s->pb, 1, 0); /* no custom non intra matrix */



            put_header(s, GOP_START_CODE);

            put_bits(&s->pb, 1, 0); /* do drop frame */

            /* time code : we must convert from the real frame rate to a

               fake mpeg frame rate in case of low frame rate */

            fps = frame_rate_tab[s->frame_rate_index];

            time_code = s->fake_picture_number * FRAME_RATE_BASE;

            s->gop_picture_number = s->fake_picture_number;

            put_bits(&s->pb, 5, (UINT32)((time_code / (fps * 3600)) % 24));

            put_bits(&s->pb, 6, (UINT32)((time_code / (fps * 60)) % 60));

            put_bits(&s->pb, 1, 1);

            put_bits(&s->pb, 6, (UINT32)((time_code / fps) % 60));

            put_bits(&s->pb, 6, (UINT32)((time_code % fps) / FRAME_RATE_BASE));

            put_bits(&s->pb, 1, 1); /* closed gop */

            put_bits(&s->pb, 1, 0); /* broken link */

        }



        if (s->frame_rate < (24 * FRAME_RATE_BASE) && s->picture_number > 0) {

            /* insert empty P pictures to slow down to the desired

               frame rate. Each fake pictures takes about 20 bytes */

            fps = frame_rate_tab[s->frame_rate_index];

            n = ((s->picture_number * fps) / s->frame_rate) - 1;

            while (s->fake_picture_number < n) {

                mpeg1_skip_picture(s, s->fake_picture_number - 

                                   s->gop_picture_number); 

                s->fake_picture_number++;

            }



        }

        s->fake_picture_number++;

}
