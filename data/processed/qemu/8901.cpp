static void nbd_trip(void *opaque)

{

    NBDClient *client = opaque;

    NBDExport *exp = client->exp;

    NBDRequest *req;

    struct nbd_request request;

    struct nbd_reply reply;

    ssize_t ret;

    uint32_t command;



    TRACE("Reading request.");

    if (client->closing) {

        return;

    }



    req = nbd_request_get(client);

    ret = nbd_co_receive_request(req, &request);

    if (ret == -EAGAIN) {

        goto done;

    }

    if (ret == -EIO) {

        goto out;

    }



    reply.handle = request.handle;

    reply.error = 0;



    if (ret < 0) {

        reply.error = -ret;

        goto error_reply;

    }

    command = request.type & NBD_CMD_MASK_COMMAND;

    if (command != NBD_CMD_DISC && (request.from + request.len) > exp->size) {

            LOG("From: %" PRIu64 ", Len: %u, Size: %" PRIu64

            ", Offset: %" PRIu64 "\n",

                    request.from, request.len,

                    (uint64_t)exp->size, (uint64_t)exp->dev_offset);

        LOG("requested operation past EOF--bad client?");

        goto invalid_request;

    }



    if (client->closing) {

        /*

         * The client may be closed when we are blocked in

         * nbd_co_receive_request()

         */

        goto done;

    }



    switch (command) {

    case NBD_CMD_READ:

        TRACE("Request type is READ");



        if (request.type & NBD_CMD_FLAG_FUA) {

            ret = blk_co_flush(exp->blk);

            if (ret < 0) {

                LOG("flush failed");

                reply.error = -ret;

                goto error_reply;

            }

        }



        ret = blk_read(exp->blk,

                       (request.from + exp->dev_offset) / BDRV_SECTOR_SIZE,

                       req->data, request.len / BDRV_SECTOR_SIZE);

        if (ret < 0) {

            LOG("reading from file failed");

            reply.error = -ret;

            goto error_reply;

        }



        TRACE("Read %u byte(s)", request.len);

        if (nbd_co_send_reply(req, &reply, request.len) < 0)

            goto out;

        break;

    case NBD_CMD_WRITE:

        TRACE("Request type is WRITE");



        if (exp->nbdflags & NBD_FLAG_READ_ONLY) {

            TRACE("Server is read-only, return error");

            reply.error = EROFS;

            goto error_reply;

        }



        TRACE("Writing to device");



        ret = blk_write(exp->blk,

                        (request.from + exp->dev_offset) / BDRV_SECTOR_SIZE,

                        req->data, request.len / BDRV_SECTOR_SIZE);

        if (ret < 0) {

            LOG("writing to file failed");

            reply.error = -ret;

            goto error_reply;

        }



        if (request.type & NBD_CMD_FLAG_FUA) {

            ret = blk_co_flush(exp->blk);

            if (ret < 0) {

                LOG("flush failed");

                reply.error = -ret;

                goto error_reply;

            }

        }



        if (nbd_co_send_reply(req, &reply, 0) < 0) {

            goto out;

        }

        break;

    case NBD_CMD_DISC:

        TRACE("Request type is DISCONNECT");

        errno = 0;

        goto out;

    case NBD_CMD_FLUSH:

        TRACE("Request type is FLUSH");



        ret = blk_co_flush(exp->blk);

        if (ret < 0) {

            LOG("flush failed");

            reply.error = -ret;

        }

        if (nbd_co_send_reply(req, &reply, 0) < 0) {

            goto out;

        }

        break;

    case NBD_CMD_TRIM:

        TRACE("Request type is TRIM");

        ret = blk_co_discard(exp->blk, (request.from + exp->dev_offset)

                                       / BDRV_SECTOR_SIZE,

                             request.len / BDRV_SECTOR_SIZE);

        if (ret < 0) {

            LOG("discard failed");

            reply.error = -ret;

        }

        if (nbd_co_send_reply(req, &reply, 0) < 0) {

            goto out;

        }

        break;

    default:

        LOG("invalid request type (%u) received", request.type);

    invalid_request:

        reply.error = EINVAL;

    error_reply:

        if (nbd_co_send_reply(req, &reply, 0) < 0) {

            goto out;

        }

        break;

    }



    TRACE("Request/Reply complete");



done:

    nbd_request_put(req);

    return;



out:

    nbd_request_put(req);

    client_close(client);

}
