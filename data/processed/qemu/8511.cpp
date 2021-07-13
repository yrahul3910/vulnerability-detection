static int bochs_open(BlockDriverState *bs, int flags)

{

    BDRVBochsState *s = bs->opaque;

    int i;

    struct bochs_header bochs;

    struct bochs_header_v1 header_v1;



    bs->read_only = 1; // no write support yet



    if (bdrv_pread(bs->file, 0, &bochs, sizeof(bochs)) != sizeof(bochs)) {

        goto fail;

    }



    if (strcmp(bochs.magic, HEADER_MAGIC) ||

        strcmp(bochs.type, REDOLOG_TYPE) ||

        strcmp(bochs.subtype, GROWING_TYPE) ||

	((le32_to_cpu(bochs.version) != HEADER_VERSION) &&

	(le32_to_cpu(bochs.version) != HEADER_V1))) {

        return -EMEDIUMTYPE;

    }



    if (le32_to_cpu(bochs.version) == HEADER_V1) {

      memcpy(&header_v1, &bochs, sizeof(bochs));

      bs->total_sectors = le64_to_cpu(header_v1.extra.redolog.disk) / 512;

    } else {

      bs->total_sectors = le64_to_cpu(bochs.extra.redolog.disk) / 512;

    }



    s->catalog_size = le32_to_cpu(bochs.extra.redolog.catalog);

    s->catalog_bitmap = g_malloc(s->catalog_size * 4);

    if (bdrv_pread(bs->file, le32_to_cpu(bochs.header), s->catalog_bitmap,

                   s->catalog_size * 4) != s->catalog_size * 4)

	goto fail;

    for (i = 0; i < s->catalog_size; i++)

	le32_to_cpus(&s->catalog_bitmap[i]);



    s->data_offset = le32_to_cpu(bochs.header) + (s->catalog_size * 4);



    s->bitmap_blocks = 1 + (le32_to_cpu(bochs.extra.redolog.bitmap) - 1) / 512;

    s->extent_blocks = 1 + (le32_to_cpu(bochs.extra.redolog.extent) - 1) / 512;



    s->extent_size = le32_to_cpu(bochs.extra.redolog.extent);



    qemu_co_mutex_init(&s->lock);

    return 0;

 fail:

    return -1;

}
