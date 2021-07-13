static int aiff_read_header(AVFormatContext *s)

{

    int ret, size, filesize;

    int64_t offset = 0, position;

    uint32_t tag;

    unsigned version = AIFF_C_VERSION1;

    AVIOContext *pb = s->pb;

    AVStream * st;

    AIFFInputContext *aiff = s->priv_data;

    ID3v2ExtraMeta *id3v2_extra_meta = NULL;



    /* check FORM header */

    filesize = get_tag(pb, &tag);

    if (filesize < 0 || tag != MKTAG('F', 'O', 'R', 'M'))

        return AVERROR_INVALIDDATA;



    /* AIFF data type */

    tag = avio_rl32(pb);

    if (tag == MKTAG('A', 'I', 'F', 'F'))       /* Got an AIFF file */

        version = AIFF;

    else if (tag != MKTAG('A', 'I', 'F', 'C'))  /* An AIFF-C file then */

        return AVERROR_INVALIDDATA;



    filesize -= 4;



    st = avformat_new_stream(s, NULL);

    if (!st)

        return AVERROR(ENOMEM);



    while (filesize > 0) {

        /* parse different chunks */

        size = get_tag(pb, &tag);



        if (size == AVERROR_EOF && offset > 0 && st->codecpar->block_align) {

            av_log(s, AV_LOG_WARNING, "header parser hit EOF\n");

            goto got_sound;

        }

        if (size < 0)

            return size;



        filesize -= size + 8;



        switch (tag) {

        case MKTAG('C', 'O', 'M', 'M'):     /* Common chunk */

            /* Then for the complete header info */

            st->nb_frames = get_aiff_header(s, size, version);

            if (st->nb_frames < 0)

                return st->nb_frames;

            if (offset > 0) // COMM is after SSND

                goto got_sound;

            break;

        case MKTAG('I', 'D', '3', ' '):

            position = avio_tell(pb);

            ff_id3v2_read(s, ID3v2_DEFAULT_MAGIC, &id3v2_extra_meta, size);

            if (id3v2_extra_meta)

                if ((ret = ff_id3v2_parse_apic(s, &id3v2_extra_meta)) < 0) {

                    ff_id3v2_free_extra_meta(&id3v2_extra_meta);

                    return ret;

                }

            ff_id3v2_free_extra_meta(&id3v2_extra_meta);

            if (position + size > avio_tell(pb))

                avio_skip(pb, position + size - avio_tell(pb));

            break;

        case MKTAG('F', 'V', 'E', 'R'):     /* Version chunk */

            version = avio_rb32(pb);

            break;

        case MKTAG('N', 'A', 'M', 'E'):     /* Sample name chunk */

            get_meta(s, "title"    , size);

            break;

        case MKTAG('A', 'U', 'T', 'H'):     /* Author chunk */

            get_meta(s, "author"   , size);

            break;

        case MKTAG('(', 'c', ')', ' '):     /* Copyright chunk */

            get_meta(s, "copyright", size);

            break;

        case MKTAG('A', 'N', 'N', 'O'):     /* Annotation chunk */

            get_meta(s, "comment"  , size);

            break;

        case MKTAG('S', 'S', 'N', 'D'):     /* Sampled sound chunk */

            aiff->data_end = avio_tell(pb) + size;

            offset = avio_rb32(pb);      /* Offset of sound data */

            avio_rb32(pb);               /* BlockSize... don't care */

            offset += avio_tell(pb);    /* Compute absolute data offset */

            if (st->codecpar->block_align && !pb->seekable)    /* Assume COMM already parsed */

                goto got_sound;

            if (!pb->seekable) {

                av_log(s, AV_LOG_ERROR, "file is not seekable\n");

                return -1;

            }

            avio_skip(pb, size - 8);

            break;

        case MKTAG('w', 'a', 'v', 'e'):

            if ((uint64_t)size > (1<<30))

                return -1;

            if (ff_get_extradata(s, st->codecpar, pb, size) < 0)

                return AVERROR(ENOMEM);

            if (   (st->codecpar->codec_id == AV_CODEC_ID_QDMC || st->codecpar->codec_id == AV_CODEC_ID_QDM2)

                && size>=12*4 && !st->codecpar->block_align) {

                st->codecpar->block_align = AV_RB32(st->codecpar->extradata+11*4);

                aiff->block_duration = AV_RB32(st->codecpar->extradata+9*4);

            } else if (st->codecpar->codec_id == AV_CODEC_ID_QCELP) {

                char rate = 0;

                if (size >= 25)

                    rate = st->codecpar->extradata[24];

                switch (rate) {

                case 'H': // RATE_HALF

                    st->codecpar->block_align = 17;

                    break;

                case 'F': // RATE_FULL

                default:

                    st->codecpar->block_align = 35;

                }

                aiff->block_duration = 160;

                st->codecpar->bit_rate = st->codecpar->sample_rate * (st->codecpar->block_align << 3) /

                                         aiff->block_duration;

            }

            break;

        case MKTAG('C','H','A','N'):

            if(ff_mov_read_chan(s, pb, st, size) < 0)

                return AVERROR_INVALIDDATA;

            break;

        case 0:

            if (offset > 0 && st->codecpar->block_align) // COMM && SSND

                goto got_sound;

        default: /* Jump */

            if (size & 1)   /* Always even aligned */

                size++;

            avio_skip(pb, size);

        }

    }



got_sound:

    if (!st->codecpar->block_align && st->codecpar->codec_id == AV_CODEC_ID_QCELP) {

        av_log(s, AV_LOG_WARNING, "qcelp without wave chunk, assuming full rate\n");

        st->codecpar->block_align = 35;

    } else if (!st->codecpar->block_align) {

        av_log(s, AV_LOG_ERROR, "could not find COMM tag or invalid block_align value\n");

        return -1;

    }



    /* Now positioned, get the sound data start and end */

    avpriv_set_pts_info(st, 64, 1, st->codecpar->sample_rate);

    st->start_time = 0;

    st->duration = st->nb_frames * aiff->block_duration;



    /* Position the stream at the first block */

    avio_seek(pb, offset, SEEK_SET);



    return 0;

}
