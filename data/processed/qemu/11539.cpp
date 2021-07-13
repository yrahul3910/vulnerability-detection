static int vncws_start_tls_handshake(struct VncState *vs)
{
    int ret = gnutls_handshake(vs->tls.session);
    if (ret < 0) {
        if (!gnutls_error_is_fatal(ret)) {
            VNC_DEBUG("Handshake interrupted (blocking)\n");
            if (!gnutls_record_get_direction(vs->tls.session)) {
                qemu_set_fd_handler(vs->csock, vncws_tls_handshake_io,
                                    NULL, vs);
                qemu_set_fd_handler(vs->csock, NULL, vncws_tls_handshake_io,
                                    vs);
            return 0;
        VNC_DEBUG("Handshake failed %s\n", gnutls_strerror(ret));
    VNC_DEBUG("Handshake done, switching to TLS data mode\n");
    qemu_set_fd_handler2(vs->csock, NULL, vncws_handshake_read, NULL, vs);
    return 0;