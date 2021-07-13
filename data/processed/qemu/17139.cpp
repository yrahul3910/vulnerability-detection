static int dmg_read_mish_block(BDRVDMGState *s, DmgHeaderState *ds,

                               uint8_t *buffer, uint32_t count)

{

    uint32_t type, i;

    int ret;

    size_t new_size;

    uint32_t chunk_count;

    int64_t offset = 0;

    uint64_t data_offset;

    uint64_t in_offset = ds->data_fork_offset;

    uint64_t out_offset;



    type = buff_read_uint32(buffer, offset);

    /* skip data that is not a valid MISH block (invalid magic or too small) */

    if (type != 0x6d697368 || count < 244) {

        /* assume success for now */

        return 0;

    }



    /* chunk offsets are relative to this sector number */

    out_offset = buff_read_uint64(buffer, offset + 8);



    /* location in data fork for (compressed) blob (in bytes) */

    data_offset = buff_read_uint64(buffer, offset + 0x18);

    in_offset += data_offset;



    /* move to begin of chunk entries */

    offset += 204;



    chunk_count = (count - 204) / 40;

    new_size = sizeof(uint64_t) * (s->n_chunks + chunk_count);

    s->types = g_realloc(s->types, new_size / 2);

    s->offsets = g_realloc(s->offsets, new_size);

    s->lengths = g_realloc(s->lengths, new_size);

    s->sectors = g_realloc(s->sectors, new_size);

    s->sectorcounts = g_realloc(s->sectorcounts, new_size);



    for (i = s->n_chunks; i < s->n_chunks + chunk_count; i++) {

        s->types[i] = buff_read_uint32(buffer, offset);

        offset += 4;

        if (s->types[i] != 0x80000005 && s->types[i] != 1 &&

            s->types[i] != 2) {

            chunk_count--;

            i--;

            offset += 36;

            continue;

        }

        offset += 4;



        s->sectors[i] = buff_read_uint64(buffer, offset);

        s->sectors[i] += out_offset;

        offset += 8;



        s->sectorcounts[i] = buff_read_uint64(buffer, offset);

        offset += 8;



        if (s->sectorcounts[i] > DMG_SECTORCOUNTS_MAX) {

            error_report("sector count %" PRIu64 " for chunk %" PRIu32

                         " is larger than max (%u)",

                         s->sectorcounts[i], i, DMG_SECTORCOUNTS_MAX);

            ret = -EINVAL;

            goto fail;

        }



        s->offsets[i] = buff_read_uint64(buffer, offset);

        s->offsets[i] += in_offset;

        offset += 8;



        s->lengths[i] = buff_read_uint64(buffer, offset);

        offset += 8;



        if (s->lengths[i] > DMG_LENGTHS_MAX) {

            error_report("length %" PRIu64 " for chunk %" PRIu32

                         " is larger than max (%u)",

                         s->lengths[i], i, DMG_LENGTHS_MAX);

            ret = -EINVAL;

            goto fail;

        }



        update_max_chunk_size(s, i, &ds->max_compressed_size,

                              &ds->max_sectors_per_chunk);

    }

    s->n_chunks += chunk_count;

    return 0;



fail:

    return ret;

}
