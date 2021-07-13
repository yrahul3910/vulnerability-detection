static int qcow2_write_snapshots(BlockDriverState *bs)

{

    BDRVQcowState *s = bs->opaque;

    QCowSnapshot *sn;

    QCowSnapshotHeader h;

    QCowSnapshotExtraData extra;

    int i, name_size, id_str_size, snapshots_size;

    struct {

        uint32_t nb_snapshots;

        uint64_t snapshots_offset;

    } QEMU_PACKED header_data;

    int64_t offset, snapshots_offset;

    int ret;



    /* compute the size of the snapshots */

    offset = 0;

    for(i = 0; i < s->nb_snapshots; i++) {

        sn = s->snapshots + i;

        offset = align_offset(offset, 8);

        offset += sizeof(h);

        offset += sizeof(extra);

        offset += strlen(sn->id_str);

        offset += strlen(sn->name);

    }

    snapshots_size = offset;



    /* Allocate space for the new snapshot list */

    snapshots_offset = qcow2_alloc_clusters(bs, snapshots_size);

    bdrv_flush(bs->file);

    offset = snapshots_offset;

    if (offset < 0) {

        return offset;

    }



    /* Write all snapshots to the new list */

    for(i = 0; i < s->nb_snapshots; i++) {

        sn = s->snapshots + i;

        memset(&h, 0, sizeof(h));

        h.l1_table_offset = cpu_to_be64(sn->l1_table_offset);

        h.l1_size = cpu_to_be32(sn->l1_size);

        /* If it doesn't fit in 32 bit, older implementations should treat it

         * as a disk-only snapshot rather than truncate the VM state */

        if (sn->vm_state_size <= 0xffffffff) {

            h.vm_state_size = cpu_to_be32(sn->vm_state_size);

        }

        h.date_sec = cpu_to_be32(sn->date_sec);

        h.date_nsec = cpu_to_be32(sn->date_nsec);

        h.vm_clock_nsec = cpu_to_be64(sn->vm_clock_nsec);

        h.extra_data_size = cpu_to_be32(sizeof(extra));



        memset(&extra, 0, sizeof(extra));

        extra.vm_state_size_large = cpu_to_be64(sn->vm_state_size);

        extra.disk_size = cpu_to_be64(sn->disk_size);



        id_str_size = strlen(sn->id_str);

        name_size = strlen(sn->name);

        h.id_str_size = cpu_to_be16(id_str_size);

        h.name_size = cpu_to_be16(name_size);

        offset = align_offset(offset, 8);



        ret = bdrv_pwrite(bs->file, offset, &h, sizeof(h));

        if (ret < 0) {

            goto fail;

        }

        offset += sizeof(h);



        ret = bdrv_pwrite(bs->file, offset, &extra, sizeof(extra));

        if (ret < 0) {

            goto fail;

        }

        offset += sizeof(extra);



        ret = bdrv_pwrite(bs->file, offset, sn->id_str, id_str_size);

        if (ret < 0) {

            goto fail;

        }

        offset += id_str_size;



        ret = bdrv_pwrite(bs->file, offset, sn->name, name_size);

        if (ret < 0) {

            goto fail;

        }

        offset += name_size;

    }



    /*

     * Update the header to point to the new snapshot table. This requires the

     * new table and its refcounts to be stable on disk.

     */

    ret = bdrv_flush(bs);

    if (ret < 0) {

        goto fail;

    }



    QEMU_BUILD_BUG_ON(offsetof(QCowHeader, snapshots_offset) !=

        offsetof(QCowHeader, nb_snapshots) + sizeof(header_data.nb_snapshots));



    header_data.nb_snapshots        = cpu_to_be32(s->nb_snapshots);

    header_data.snapshots_offset    = cpu_to_be64(snapshots_offset);



    ret = bdrv_pwrite_sync(bs->file, offsetof(QCowHeader, nb_snapshots),

                           &header_data, sizeof(header_data));

    if (ret < 0) {

        goto fail;

    }



    /* free the old snapshot table */

    qcow2_free_clusters(bs, s->snapshots_offset, s->snapshots_size);

    s->snapshots_offset = snapshots_offset;

    s->snapshots_size = snapshots_size;

    return 0;



fail:

    return ret;

}
