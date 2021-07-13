static int create_vorbis_context(vorbis_enc_context *venc,

                                 AVCodecContext *avctx)

{

    vorbis_enc_floor   *fc;

    vorbis_enc_residue *rc;

    vorbis_enc_mapping *mc;

    int i, book, ret;



    venc->channels    = avctx->channels;

    venc->sample_rate = avctx->sample_rate;

    venc->log2_blocksize[0] = venc->log2_blocksize[1] = 11;



    venc->ncodebooks = FF_ARRAY_ELEMS(cvectors);

    venc->codebooks  = av_malloc(sizeof(vorbis_enc_codebook) * venc->ncodebooks);

    if (!venc->codebooks)

        return AVERROR(ENOMEM);



    // codebook 0..14 - floor1 book, values 0..255

    // codebook 15 residue masterbook

    // codebook 16..29 residue

    for (book = 0; book < venc->ncodebooks; book++) {

        vorbis_enc_codebook *cb = &venc->codebooks[book];

        int vals;

        cb->ndimensions = cvectors[book].dim;

        cb->nentries    = cvectors[book].real_len;

        cb->min         = cvectors[book].min;

        cb->delta       = cvectors[book].delta;

        cb->lookup      = cvectors[book].lookup;

        cb->seq_p       = 0;



        cb->lens      = av_malloc_array(cb->nentries, sizeof(uint8_t));

        cb->codewords = av_malloc_array(cb->nentries, sizeof(uint32_t));

        if (!cb->lens || !cb->codewords)

            return AVERROR(ENOMEM);

        memcpy(cb->lens, cvectors[book].clens, cvectors[book].len);

        memset(cb->lens + cvectors[book].len, 0, cb->nentries - cvectors[book].len);



        if (cb->lookup) {

            vals = cb_lookup_vals(cb->lookup, cb->ndimensions, cb->nentries);

            cb->quantlist = av_malloc_array(vals, sizeof(int));

            if (!cb->quantlist)

                return AVERROR(ENOMEM);

            for (i = 0; i < vals; i++)

                cb->quantlist[i] = cvectors[book].quant[i];

        } else {

            cb->quantlist = NULL;

        }

        if ((ret = ready_codebook(cb)) < 0)

            return ret;

    }



    venc->nfloors = 1;

    venc->floors  = av_malloc(sizeof(vorbis_enc_floor) * venc->nfloors);

    if (!venc->floors)

        return AVERROR(ENOMEM);



    // just 1 floor

    fc = &venc->floors[0];

    fc->partitions         = NUM_FLOOR_PARTITIONS;

    fc->partition_to_class = av_malloc(sizeof(int) * fc->partitions);

    if (!fc->partition_to_class)

        return AVERROR(ENOMEM);

    fc->nclasses           = 0;

    for (i = 0; i < fc->partitions; i++) {

        static const int a[] = {0, 1, 2, 2, 3, 3, 4, 4};

        fc->partition_to_class[i] = a[i];

        fc->nclasses = FFMAX(fc->nclasses, fc->partition_to_class[i]);

    }

    fc->nclasses++;

    fc->classes = av_malloc_array(fc->nclasses, sizeof(vorbis_enc_floor_class));

    if (!fc->classes)

        return AVERROR(ENOMEM);

    for (i = 0; i < fc->nclasses; i++) {

        vorbis_enc_floor_class * c = &fc->classes[i];

        int j, books;

        c->dim        = floor_classes[i].dim;

        c->subclass   = floor_classes[i].subclass;

        c->masterbook = floor_classes[i].masterbook;

        books         = (1 << c->subclass);

        c->books      = av_malloc_array(books, sizeof(int));

        if (!c->books)

            return AVERROR(ENOMEM);

        for (j = 0; j < books; j++)

            c->books[j] = floor_classes[i].nbooks[j];

    }

    fc->multiplier = 2;

    fc->rangebits  = venc->log2_blocksize[0] - 1;



    fc->values = 2;

    for (i = 0; i < fc->partitions; i++)

        fc->values += fc->classes[fc->partition_to_class[i]].dim;



    fc->list = av_malloc_array(fc->values, sizeof(vorbis_floor1_entry));

    if (!fc->list)

        return AVERROR(ENOMEM);

    fc->list[0].x = 0;

    fc->list[1].x = 1 << fc->rangebits;

    for (i = 2; i < fc->values; i++) {

        static const int a[] = {

             93, 23,372,  6, 46,186,750, 14, 33, 65,

            130,260,556,  3, 10, 18, 28, 39, 55, 79,

            111,158,220,312,464,650,850

        };

        fc->list[i].x = a[i - 2];

    }

    if (ff_vorbis_ready_floor1_list(avctx, fc->list, fc->values))

        return AVERROR_BUG;



    venc->nresidues = 1;

    venc->residues  = av_malloc(sizeof(vorbis_enc_residue) * venc->nresidues);

    if (!venc->residues)

        return AVERROR(ENOMEM);



    // single residue

    rc = &venc->residues[0];

    rc->type            = 2;

    rc->begin           = 0;

    rc->end             = 1600;

    rc->partition_size  = 32;

    rc->classifications = 10;

    rc->classbook       = 15;

    rc->books           = av_malloc(sizeof(*rc->books) * rc->classifications);

    if (!rc->books)

        return AVERROR(ENOMEM);

    {

        static const int8_t a[10][8] = {

            { -1, -1, -1, -1, -1, -1, -1, -1, },

            { -1, -1, 16, -1, -1, -1, -1, -1, },

            { -1, -1, 17, -1, -1, -1, -1, -1, },

            { -1, -1, 18, -1, -1, -1, -1, -1, },

            { -1, -1, 19, -1, -1, -1, -1, -1, },

            { -1, -1, 20, -1, -1, -1, -1, -1, },

            { -1, -1, 21, -1, -1, -1, -1, -1, },

            { 22, 23, -1, -1, -1, -1, -1, -1, },

            { 24, 25, -1, -1, -1, -1, -1, -1, },

            { 26, 27, 28, -1, -1, -1, -1, -1, },

        };

        memcpy(rc->books, a, sizeof a);

    }

    if ((ret = ready_residue(rc, venc)) < 0)

        return ret;



    venc->nmappings = 1;

    venc->mappings  = av_malloc(sizeof(vorbis_enc_mapping) * venc->nmappings);

    if (!venc->mappings)

        return AVERROR(ENOMEM);



    // single mapping

    mc = &venc->mappings[0];

    mc->submaps = 1;

    mc->mux     = av_malloc(sizeof(int) * venc->channels);

    if (!mc->mux)

        return AVERROR(ENOMEM);

    for (i = 0; i < venc->channels; i++)

        mc->mux[i] = 0;

    mc->floor   = av_malloc(sizeof(int) * mc->submaps);

    mc->residue = av_malloc(sizeof(int) * mc->submaps);

    if (!mc->floor || !mc->residue)

        return AVERROR(ENOMEM);

    for (i = 0; i < mc->submaps; i++) {

        mc->floor[i]   = 0;

        mc->residue[i] = 0;

    }

    mc->coupling_steps = venc->channels == 2 ? 1 : 0;

    mc->magnitude      = av_malloc(sizeof(int) * mc->coupling_steps);

    mc->angle          = av_malloc(sizeof(int) * mc->coupling_steps);

    if (!mc->magnitude || !mc->angle)

        return AVERROR(ENOMEM);

    if (mc->coupling_steps) {

        mc->magnitude[0] = 0;

        mc->angle[0]     = 1;

    }



    venc->nmodes = 1;

    venc->modes  = av_malloc(sizeof(vorbis_enc_mode) * venc->nmodes);

    if (!venc->modes)

        return AVERROR(ENOMEM);



    // single mode

    venc->modes[0].blockflag = 0;

    venc->modes[0].mapping   = 0;



    venc->have_saved = 0;

    venc->saved      = av_malloc_array(sizeof(float) * venc->channels, (1 << venc->log2_blocksize[1]) / 2);

    venc->samples    = av_malloc_array(sizeof(float) * venc->channels, (1 << venc->log2_blocksize[1]));

    venc->floor      = av_malloc_array(sizeof(float) * venc->channels, (1 << venc->log2_blocksize[1]) / 2);

    venc->coeffs     = av_malloc_array(sizeof(float) * venc->channels, (1 << venc->log2_blocksize[1]) / 2);

    venc->scratch    = av_malloc_array(sizeof(float) * venc->channels, (1 << venc->log2_blocksize[1]) / 2);



    if (!venc->saved || !venc->samples || !venc->floor || !venc->coeffs || !venc->scratch)

        return AVERROR(ENOMEM);



    if ((ret = dsp_init(avctx, venc)) < 0)

        return ret;



    return 0;

}
