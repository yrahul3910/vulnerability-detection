int coroutine_fn bdrv_co_pwritev(BlockDriverState *bs,

    int64_t offset, unsigned int bytes, QEMUIOVector *qiov,

    BdrvRequestFlags flags)

{

    BdrvTrackedRequest req;

    uint64_t align = bs->request_alignment;

    uint8_t *head_buf = NULL;

    uint8_t *tail_buf = NULL;

    QEMUIOVector local_qiov;

    bool use_local_qiov = false;

    int ret;



    if (!bs->drv) {

        return -ENOMEDIUM;

    }

    if (bs->read_only) {

        return -EPERM;

    }

    assert(!(bs->open_flags & BDRV_O_INACTIVE));



    ret = bdrv_check_byte_request(bs, offset, bytes);

    if (ret < 0) {

        return ret;

    }



    /*

     * Align write if necessary by performing a read-modify-write cycle.

     * Pad qiov with the read parts and be sure to have a tracked request not

     * only for bdrv_aligned_pwritev, but also for the reads of the RMW cycle.

     */

    tracked_request_begin(&req, bs, offset, bytes, BDRV_TRACKED_WRITE);



    if (!qiov) {

        ret = bdrv_co_do_zero_pwritev(bs, offset, bytes, flags, &req);

        goto out;

    }



    if (offset & (align - 1)) {

        QEMUIOVector head_qiov;

        struct iovec head_iov;



        mark_request_serialising(&req, align);

        wait_serialising_requests(&req);



        head_buf = qemu_blockalign(bs, align);

        head_iov = (struct iovec) {

            .iov_base   = head_buf,

            .iov_len    = align,

        };

        qemu_iovec_init_external(&head_qiov, &head_iov, 1);



        bdrv_debug_event(bs, BLKDBG_PWRITEV_RMW_HEAD);

        ret = bdrv_aligned_preadv(bs, &req, offset & ~(align - 1), align,

                                  align, &head_qiov, 0);

        if (ret < 0) {

            goto fail;

        }

        bdrv_debug_event(bs, BLKDBG_PWRITEV_RMW_AFTER_HEAD);



        qemu_iovec_init(&local_qiov, qiov->niov + 2);

        qemu_iovec_add(&local_qiov, head_buf, offset & (align - 1));

        qemu_iovec_concat(&local_qiov, qiov, 0, qiov->size);

        use_local_qiov = true;



        bytes += offset & (align - 1);

        offset = offset & ~(align - 1);



        /* We have read the tail already if the request is smaller

         * than one aligned block.

         */

        if (bytes < align) {

            qemu_iovec_add(&local_qiov, head_buf + bytes, align - bytes);

            bytes = align;

        }

    }



    if ((offset + bytes) & (align - 1)) {

        QEMUIOVector tail_qiov;

        struct iovec tail_iov;

        size_t tail_bytes;

        bool waited;



        mark_request_serialising(&req, align);

        waited = wait_serialising_requests(&req);

        assert(!waited || !use_local_qiov);



        tail_buf = qemu_blockalign(bs, align);

        tail_iov = (struct iovec) {

            .iov_base   = tail_buf,

            .iov_len    = align,

        };

        qemu_iovec_init_external(&tail_qiov, &tail_iov, 1);



        bdrv_debug_event(bs, BLKDBG_PWRITEV_RMW_TAIL);

        ret = bdrv_aligned_preadv(bs, &req, (offset + bytes) & ~(align - 1), align,

                                  align, &tail_qiov, 0);

        if (ret < 0) {

            goto fail;

        }

        bdrv_debug_event(bs, BLKDBG_PWRITEV_RMW_AFTER_TAIL);



        if (!use_local_qiov) {

            qemu_iovec_init(&local_qiov, qiov->niov + 1);

            qemu_iovec_concat(&local_qiov, qiov, 0, qiov->size);

            use_local_qiov = true;

        }



        tail_bytes = (offset + bytes) & (align - 1);

        qemu_iovec_add(&local_qiov, tail_buf + tail_bytes, align - tail_bytes);



        bytes = ROUND_UP(bytes, align);

    }



    ret = bdrv_aligned_pwritev(bs, &req, offset, bytes, align,

                               use_local_qiov ? &local_qiov : qiov,

                               flags);



fail:



    if (use_local_qiov) {

        qemu_iovec_destroy(&local_qiov);

    }

    qemu_vfree(head_buf);

    qemu_vfree(tail_buf);

out:

    tracked_request_end(&req);

    return ret;

}
