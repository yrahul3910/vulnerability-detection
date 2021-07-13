static int bochs_open(BlockDriverState *bs, QDict *options, int flags,

                      Error **errp)

{

    BDRVBochsState *s = bs->opaque;

    uint32_t i;

    struct bochs_header bochs;

    int ret;



    bs->file = bdrv_open_child(NULL, options, "file", bs, &child_file,

                               false, errp);

    if (!bs->file) {

        return -EINVAL;

    }



    bdrv_set_read_only(bs, true); /* no write support yet */



    ret = bdrv_pread(bs->file, 0, &bochs, sizeof(bochs));

    if (ret < 0) {

        return ret;

    }



    if (strcmp(bochs.magic, HEADER_MAGIC) ||

        strcmp(bochs.type, REDOLOG_TYPE) ||

        strcmp(bochs.subtype, GROWING_TYPE) ||

	((le32_to_cpu(bochs.version) != HEADER_VERSION) &&

	(le32_to_cpu(bochs.version) != HEADER_V1))) {

        error_setg(errp, "Image not in Bochs format");

        return -EINVAL;

    }



    if (le32_to_cpu(bochs.version) == HEADER_V1) {

        bs->total_sectors = le64_to_cpu(bochs.extra.redolog_v1.disk) / 512;

    } else {

        bs->total_sectors = le64_to_cpu(bochs.extra.redolog.disk) / 512;

    }



    /* Limit to 1M entries to avoid unbounded allocation. This is what is

     * needed for the largest image that bximage can create (~8 TB). */

    s->catalog_size = le32_to_cpu(bochs.catalog);

    if (s->catalog_size > 0x100000) {

        error_setg(errp, "Catalog size is too large");

        return -EFBIG;

    }



    s->catalog_bitmap = g_try_new(uint32_t, s->catalog_size);

    if (s->catalog_size && s->catalog_bitmap == NULL) {

        error_setg(errp, "Could not allocate memory for catalog");

        return -ENOMEM;

    }



    ret = bdrv_pread(bs->file, le32_to_cpu(bochs.header), s->catalog_bitmap,

                     s->catalog_size * 4);

    if (ret < 0) {

        goto fail;

    }



    for (i = 0; i < s->catalog_size; i++)

	le32_to_cpus(&s->catalog_bitmap[i]);



    s->data_offset = le32_to_cpu(bochs.header) + (s->catalog_size * 4);



    s->bitmap_blocks = 1 + (le32_to_cpu(bochs.bitmap) - 1) / 512;

    s->extent_blocks = 1 + (le32_to_cpu(bochs.extent) - 1) / 512;



    s->extent_size = le32_to_cpu(bochs.extent);

    if (s->extent_size < BDRV_SECTOR_SIZE) {

        /* bximage actually never creates extents smaller than 4k */

        error_setg(errp, "Extent size must be at least 512");

        ret = -EINVAL;

        goto fail;

    } else if (!is_power_of_2(s->extent_size)) {

        error_setg(errp, "Extent size %" PRIu32 " is not a power of two",

                   s->extent_size);

        ret = -EINVAL;

        goto fail;

    } else if (s->extent_size > 0x800000) {

        error_setg(errp, "Extent size %" PRIu32 " is too large",

                   s->extent_size);

        ret = -EINVAL;

        goto fail;

    }



    if (s->catalog_size < DIV_ROUND_UP(bs->total_sectors,

                                       s->extent_size / BDRV_SECTOR_SIZE))

    {

        error_setg(errp, "Catalog size is too small for this disk size");

        ret = -EINVAL;

        goto fail;

    }



    qemu_co_mutex_init(&s->lock);

    return 0;



fail:

    g_free(s->catalog_bitmap);

    return ret;

}
