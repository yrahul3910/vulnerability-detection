int nbd_client_init(BlockDriverState *bs, QIOChannelSocket *sioc,

                    const char *export, Error **errp)

{

    NbdClientSession *client = nbd_get_client_session(bs);

    int ret;



    /* NBD handshake */

    logout("session init %s\n", export);

    qio_channel_set_blocking(QIO_CHANNEL(sioc), true, NULL);



    ret = nbd_receive_negotiate(QIO_CHANNEL(sioc), export,

                                &client->nbdflags,

                                NULL, NULL,

                                &client->ioc,

                                &client->size, errp);

    if (ret < 0) {

        logout("Failed to negotiate with the NBD server\n");

        return ret;

    }



    qemu_co_mutex_init(&client->send_mutex);

    qemu_co_mutex_init(&client->free_sema);

    client->sioc = sioc;

    object_ref(OBJECT(client->sioc));



    if (!client->ioc) {

        client->ioc = QIO_CHANNEL(sioc);

        object_ref(OBJECT(client->ioc));

    }



    /* Now that we're connected, set the socket to be non-blocking and

     * kick the reply mechanism.  */

    qio_channel_set_blocking(QIO_CHANNEL(sioc), false, NULL);



    nbd_client_attach_aio_context(bs, bdrv_get_aio_context(bs));



    logout("Established connection with NBD server\n");

    return 0;

}
