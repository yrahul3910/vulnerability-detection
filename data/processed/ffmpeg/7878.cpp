static int RENAME(resample_common)(ResampleContext *c,

                                   DELEM *dst, const DELEM *src,

                                   int n, int update_ctx)

{

    int dst_index;

    int index= c->index;

    int frac= c->frac;

    int sample_index = index >> c->phase_shift;



    index &= c->phase_mask;

    for (dst_index = 0; dst_index < n; dst_index++) {

        FELEM *filter = ((FELEM *) c->filter_bank) + c->filter_alloc * index;



        FELEM2 val=0;

        int i;

        for (i = 0; i < c->filter_length; i++) {

            val += src[sample_index + i] * (FELEM2)filter[i];

        }

        OUT(dst[dst_index], val);



        frac  += c->dst_incr_mod;

        index += c->dst_incr_div;

        if (frac >= c->src_incr) {

            frac -= c->src_incr;

            index++;

        }

        sample_index += index >> c->phase_shift;

        index &= c->phase_mask;

    }



    if(update_ctx){

        c->frac= frac;

        c->index= index;

    }



    return sample_index;

}
