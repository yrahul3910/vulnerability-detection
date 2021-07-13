void qmp_block_commit(bool has_job_id, const char *job_id, const char *device,

                      bool has_base, const char *base,

                      bool has_top, const char *top,

                      bool has_backing_file, const char *backing_file,

                      bool has_speed, int64_t speed,

                      Error **errp)

{

    BlockDriverState *bs;

    BlockDriverState *base_bs, *top_bs;

    AioContext *aio_context;

    Error *local_err = NULL;

    /* This will be part of the QMP command, if/when the

     * BlockdevOnError change for blkmirror makes it in

     */

    BlockdevOnError on_error = BLOCKDEV_ON_ERROR_REPORT;



    if (!has_speed) {

        speed = 0;

    }



    /* Important Note:

     *  libvirt relies on the DeviceNotFound error class in order to probe for

     *  live commit feature versions; for this to work, we must make sure to

     *  perform the device lookup before any generic errors that may occur in a

     *  scenario in which all optional arguments are omitted. */

    bs = qmp_get_root_bs(device, &local_err);

    if (!bs) {

        bs = bdrv_lookup_bs(device, device, NULL);

        if (!bs) {

            error_free(local_err);

            error_set(errp, ERROR_CLASS_DEVICE_NOT_FOUND,

                      "Device '%s' not found", device);

        } else {

            error_propagate(errp, local_err);

        }

        return;

    }



    aio_context = bdrv_get_aio_context(bs);

    aio_context_acquire(aio_context);



    if (bdrv_op_is_blocked(bs, BLOCK_OP_TYPE_COMMIT_SOURCE, errp)) {

        goto out;

    }



    /* default top_bs is the active layer */

    top_bs = bs;



    if (has_top && top) {

        if (strcmp(bs->filename, top) != 0) {

            top_bs = bdrv_find_backing_image(bs, top);

        }

    }



    if (top_bs == NULL) {

        error_setg(errp, "Top image file %s not found", top ? top : "NULL");

        goto out;

    }



    assert(bdrv_get_aio_context(top_bs) == aio_context);



    if (has_base && base) {

        base_bs = bdrv_find_backing_image(top_bs, base);

    } else {

        base_bs = bdrv_find_base(top_bs);

    }



    if (base_bs == NULL) {

        error_setg(errp, QERR_BASE_NOT_FOUND, base ? base : "NULL");

        goto out;

    }



    assert(bdrv_get_aio_context(base_bs) == aio_context);



    if (bdrv_op_is_blocked(base_bs, BLOCK_OP_TYPE_COMMIT_TARGET, errp)) {

        goto out;

    }



    /* Do not allow attempts to commit an image into itself */

    if (top_bs == base_bs) {

        error_setg(errp, "cannot commit an image into itself");

        goto out;

    }



    if (top_bs == bs) {

        if (has_backing_file) {

            error_setg(errp, "'backing-file' specified,"

                             " but 'top' is the active layer");

            goto out;

        }

        commit_active_start(has_job_id ? job_id : NULL, bs, base_bs, speed,

                            on_error, block_job_cb, bs, &local_err, false);

    } else {

        commit_start(has_job_id ? job_id : NULL, bs, base_bs, top_bs, speed,

                     on_error, block_job_cb, bs,

                     has_backing_file ? backing_file : NULL, &local_err);

    }

    if (local_err != NULL) {

        error_propagate(errp, local_err);

        goto out;

    }



out:

    aio_context_release(aio_context);

}
