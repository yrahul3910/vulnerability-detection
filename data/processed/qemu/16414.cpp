static void virtio_blk_handle_write(VirtIOBlockReq *req, MultiReqBuffer *mrb)

{

    BlockRequest *blkreq;

    uint64_t sector;



    sector = virtio_ldq_p(VIRTIO_DEVICE(req->dev), &req->out.sector);



    bdrv_acct_start(req->dev->bs, &req->acct, req->qiov.size, BDRV_ACCT_WRITE);



    trace_virtio_blk_handle_write(req, sector, req->qiov.size / 512);



    if (!virtio_blk_sect_range_ok(req->dev, sector, req->qiov.size)) {

        virtio_blk_rw_complete(req, -EIO);

        return;

    }



    if (mrb->num_writes == 32) {

        virtio_submit_multiwrite(req->dev->bs, mrb);

    }



    blkreq = &mrb->blkreq[mrb->num_writes];

    blkreq->sector = sector;

    blkreq->nb_sectors = req->qiov.size / BDRV_SECTOR_SIZE;

    blkreq->qiov = &req->qiov;

    blkreq->cb = virtio_blk_rw_complete;

    blkreq->opaque = req;

    blkreq->error = 0;



    mrb->num_writes++;

}
