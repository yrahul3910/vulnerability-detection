static AVIOContext * wtvfile_open_sector(int first_sector, uint64_t length, int depth, AVFormatContext *s)

{

    AVIOContext *pb;

    WtvFile *wf;

    uint8_t *buffer;



    if (seek_by_sector(s->pb, first_sector, 0) < 0)

        return NULL;



    wf = av_mallocz(sizeof(WtvFile));

    if (!wf)

        return NULL;



    if (depth == 0) {

        wf->sectors = av_malloc(sizeof(uint32_t));

        if (!wf->sectors) {

            av_free(wf);

            return NULL;

        }

        wf->sectors[0]  = first_sector;

        wf->nb_sectors  = 1;

    } else if (depth == 1) {

        wf->sectors = av_malloc(WTV_SECTOR_SIZE);

        if (!wf->sectors) {

            av_free(wf);

            return NULL;

        }

        wf->nb_sectors  = read_ints(s->pb, wf->sectors, WTV_SECTOR_SIZE / 4);

    } else if (depth == 2) {

        uint32_t sectors1[WTV_SECTOR_SIZE / 4];

        int nb_sectors1 = read_ints(s->pb, sectors1, WTV_SECTOR_SIZE / 4);

        int i;



        wf->sectors = av_malloc_array(nb_sectors1, 1 << WTV_SECTOR_BITS);

        if (!wf->sectors) {

            av_free(wf);

            return NULL;

        }

        wf->nb_sectors = 0;

        for (i = 0; i < nb_sectors1; i++) {

            if (seek_by_sector(s->pb, sectors1[i], 0) < 0)

                break;

            wf->nb_sectors += read_ints(s->pb, wf->sectors + i * WTV_SECTOR_SIZE / 4, WTV_SECTOR_SIZE / 4);

        }

    } else {

        av_log(s, AV_LOG_ERROR, "unsupported file allocation table depth (0x%x)\n", depth);

        av_free(wf);

        return NULL;

    }

    wf->sector_bits = length & (1ULL<<63) ? WTV_SECTOR_BITS : WTV_BIGSECTOR_BITS;



    if (!wf->nb_sectors) {

        av_free(wf->sectors);

        av_free(wf);

        return NULL;

    }



    if ((int64_t)wf->sectors[wf->nb_sectors - 1] << WTV_SECTOR_BITS > avio_tell(s->pb))

        av_log(s, AV_LOG_WARNING, "truncated file\n");



    /* check length */

    length &= 0xFFFFFFFFFFFF;

    if (length > ((int64_t)wf->nb_sectors << wf->sector_bits)) {

        av_log(s, AV_LOG_WARNING, "reported file length (0x%"PRIx64") exceeds number of available sectors (0x%"PRIx64")\n", length, (int64_t)wf->nb_sectors << wf->sector_bits);

        length = (int64_t)wf->nb_sectors <<  wf->sector_bits;

    }

    wf->length = length;



    /* seek to initial sector */

    wf->position = 0;

    if (seek_by_sector(s->pb, wf->sectors[0], 0) < 0) {

        av_free(wf->sectors);

        av_free(wf);

        return NULL;

    }



    wf->pb_filesystem = s->pb;

    buffer = av_malloc(1 << wf->sector_bits);

    if (!buffer) {

        av_free(wf->sectors);

        av_free(wf);

        return NULL;

    }



    pb = avio_alloc_context(buffer, 1 << wf->sector_bits, 0, wf,

                           wtvfile_read_packet, NULL, wtvfile_seek);

    if (!pb) {

        av_free(buffer);

        av_free(wf->sectors);

        av_free(wf);

    }

    return pb;

}
