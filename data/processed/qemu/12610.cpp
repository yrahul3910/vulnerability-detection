static int coroutine_fn bdrv_co_do_pwrite_zeroes(BlockDriverState *bs,

    int64_t offset, int count, BdrvRequestFlags flags)

{

    BlockDriver *drv = bs->drv;

    QEMUIOVector qiov;

    struct iovec iov = {0};

    int ret = 0;

    bool need_flush = false;

    int head = 0;

    int tail = 0;



    int max_write_zeroes = MIN_NON_ZERO(bs->bl.max_pwrite_zeroes, INT_MAX);

    int alignment = MAX(bs->bl.pwrite_zeroes_alignment ?: 1,

                        bs->request_alignment);



    assert(is_power_of_2(alignment));

    head = offset & (alignment - 1);

    tail = (offset + count) & (alignment - 1);

    max_write_zeroes &= ~(alignment - 1);



    while (count > 0 && !ret) {

        int num = count;



        /* Align request.  Block drivers can expect the "bulk" of the request

         * to be aligned, and that unaligned requests do not cross cluster

         * boundaries.

         */

        if (head) {

            /* Make a small request up to the first aligned sector.  */

            num = MIN(count, alignment - head);

            head = 0;

        } else if (tail && num > alignment) {

            /* Shorten the request to the last aligned sector.  */

            num -= tail;

        }



        /* limit request size */

        if (num > max_write_zeroes) {

            num = max_write_zeroes;

        }



        ret = -ENOTSUP;

        /* First try the efficient write zeroes operation */

        if (drv->bdrv_co_pwrite_zeroes) {

            ret = drv->bdrv_co_pwrite_zeroes(bs, offset, num,

                                             flags & bs->supported_zero_flags);

            if (ret != -ENOTSUP && (flags & BDRV_REQ_FUA) &&

                !(bs->supported_zero_flags & BDRV_REQ_FUA)) {

                need_flush = true;

            }

        } else {

            assert(!bs->supported_zero_flags);

        }



        if (ret == -ENOTSUP) {

            /* Fall back to bounce buffer if write zeroes is unsupported */

            int max_xfer_len = MIN_NON_ZERO(bs->bl.max_transfer_length,

                                            MAX_WRITE_ZEROES_BOUNCE_BUFFER);

            BdrvRequestFlags write_flags = flags & ~BDRV_REQ_ZERO_WRITE;



            if ((flags & BDRV_REQ_FUA) &&

                !(bs->supported_write_flags & BDRV_REQ_FUA)) {

                /* No need for bdrv_driver_pwrite() to do a fallback

                 * flush on each chunk; use just one at the end */

                write_flags &= ~BDRV_REQ_FUA;

                need_flush = true;

            }

            num = MIN(num, max_xfer_len << BDRV_SECTOR_BITS);

            iov.iov_len = num;

            if (iov.iov_base == NULL) {

                iov.iov_base = qemu_try_blockalign(bs, num);

                if (iov.iov_base == NULL) {

                    ret = -ENOMEM;

                    goto fail;

                }

                memset(iov.iov_base, 0, num);

            }

            qemu_iovec_init_external(&qiov, &iov, 1);



            ret = bdrv_driver_pwritev(bs, offset, num, &qiov, write_flags);



            /* Keep bounce buffer around if it is big enough for all

             * all future requests.

             */

            if (num < max_xfer_len << BDRV_SECTOR_BITS) {

                qemu_vfree(iov.iov_base);

                iov.iov_base = NULL;

            }

        }



        offset += num;

        count -= num;

    }



fail:

    if (ret == 0 && need_flush) {

        ret = bdrv_co_flush(bs);

    }

    qemu_vfree(iov.iov_base);

    return ret;

}
