static int coroutine_fn bdrv_aligned_preadv(BlockDriverState *bs,

    BdrvTrackedRequest *req, int64_t offset, unsigned int bytes,

    int64_t align, QEMUIOVector *qiov, int flags)

{

    BlockDriver *drv = bs->drv;

    int ret;



    int64_t sector_num = offset >> BDRV_SECTOR_BITS;

    unsigned int nb_sectors = bytes >> BDRV_SECTOR_BITS;



    assert((offset & (BDRV_SECTOR_SIZE - 1)) == 0);

    assert((bytes & (BDRV_SECTOR_SIZE - 1)) == 0);



    /* Handle Copy on Read and associated serialisation */

    if (flags & BDRV_REQ_COPY_ON_READ) {

        /* If we touch the same cluster it counts as an overlap.  This

         * guarantees that allocating writes will be serialized and not race

         * with each other for the same cluster.  For example, in copy-on-read

         * it ensures that the CoR read and write operations are atomic and

         * guest writes cannot interleave between them. */

        mark_request_serialising(req, bdrv_get_cluster_size(bs));

    }



    wait_serialising_requests(req);



    if (flags & BDRV_REQ_COPY_ON_READ) {

        int pnum;



        ret = bdrv_is_allocated(bs, sector_num, nb_sectors, &pnum);

        if (ret < 0) {

            goto out;

        }



        if (!ret || pnum != nb_sectors) {

            ret = bdrv_co_do_copy_on_readv(bs, sector_num, nb_sectors, qiov);

            goto out;

        }

    }



    /* Forward the request to the BlockDriver */

    if (!(bs->zero_beyond_eof && bs->growable)) {

        ret = drv->bdrv_co_readv(bs, sector_num, nb_sectors, qiov);

    } else {

        /* Read zeros after EOF of growable BDSes */

        int64_t len, total_sectors, max_nb_sectors;



        len = bdrv_getlength(bs);

        if (len < 0) {

            ret = len;

            goto out;

        }



        total_sectors = DIV_ROUND_UP(len, BDRV_SECTOR_SIZE);

        max_nb_sectors = ROUND_UP(MAX(0, total_sectors - sector_num),

                                  align >> BDRV_SECTOR_BITS);

        if (max_nb_sectors > 0) {

            ret = drv->bdrv_co_readv(bs, sector_num,

                                     MIN(nb_sectors, max_nb_sectors), qiov);

        } else {

            ret = 0;

        }



        /* Reading beyond end of file is supposed to produce zeroes */

        if (ret == 0 && total_sectors < sector_num + nb_sectors) {

            uint64_t offset = MAX(0, total_sectors - sector_num);

            uint64_t bytes = (sector_num + nb_sectors - offset) *

                              BDRV_SECTOR_SIZE;

            qemu_iovec_memset(qiov, offset * BDRV_SECTOR_SIZE, 0, bytes);

        }

    }



out:

    return ret;

}
