static int nbd_co_receive_request(NBDRequestData *req, NBDRequest *request,

                                  Error **errp)

{

    NBDClient *client = req->client;



    g_assert(qemu_in_coroutine());

    assert(client->recv_coroutine == qemu_coroutine_self());

    if (nbd_receive_request(client->ioc, request, errp) < 0) {

        return -EIO;

    }



    trace_nbd_co_receive_request_decode_type(request->handle, request->type);



    if (request->type != NBD_CMD_WRITE) {

        /* No payload, we are ready to read the next request.  */

        req->complete = true;

    }



    if (request->type == NBD_CMD_DISC) {

        /* Special case: we're going to disconnect without a reply,

         * whether or not flags, from, or len are bogus */

        return -EIO;

    }



    /* Check for sanity in the parameters, part 1.  Defer as many

     * checks as possible until after reading any NBD_CMD_WRITE

     * payload, so we can try and keep the connection alive.  */

    if ((request->from + request->len) < request->from) {

        error_setg(errp,

                   "integer overflow detected, you're probably being attacked");

        return -EINVAL;

    }



    if (request->type == NBD_CMD_READ || request->type == NBD_CMD_WRITE) {

        if (request->len > NBD_MAX_BUFFER_SIZE) {

            error_setg(errp, "len (%" PRIu32" ) is larger than max len (%u)",

                       request->len, NBD_MAX_BUFFER_SIZE);

            return -EINVAL;

        }



        req->data = blk_try_blockalign(client->exp->blk, request->len);

        if (req->data == NULL) {

            error_setg(errp, "No memory");

            return -ENOMEM;

        }

    }

    if (request->type == NBD_CMD_WRITE) {

        if (nbd_read(client->ioc, req->data, request->len, errp) < 0) {

            error_prepend(errp, "reading from socket failed: ");

            return -EIO;

        }

        req->complete = true;



        trace_nbd_co_receive_request_payload_received(request->handle,

                                                      request->len);

    }



    /* Sanity checks, part 2. */

    if (request->from + request->len > client->exp->size) {

        error_setg(errp, "operation past EOF; From: %" PRIu64 ", Len: %" PRIu32

                   ", Size: %" PRIu64, request->from, request->len,

                   (uint64_t)client->exp->size);

        return request->type == NBD_CMD_WRITE ? -ENOSPC : -EINVAL;

    }

    if (request->flags & ~(NBD_CMD_FLAG_FUA | NBD_CMD_FLAG_NO_HOLE)) {

        error_setg(errp, "unsupported flags (got 0x%x)", request->flags);

        return -EINVAL;

    }

    if (request->type != NBD_CMD_WRITE_ZEROES &&

        (request->flags & NBD_CMD_FLAG_NO_HOLE)) {

        error_setg(errp, "unexpected flags (got 0x%x)", request->flags);

        return -EINVAL;

    }



    return 0;

}
