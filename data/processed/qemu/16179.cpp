int inet_nonblocking_connect(const char *str, bool *in_progress,

                             Error **errp)

{

    QemuOpts *opts;

    int sock = -1;



    opts = qemu_opts_create(&dummy_opts, NULL, 0, NULL);

    if (inet_parse(opts, str) == 0) {

        sock = inet_connect_opts(opts, false, in_progress, errp);

    } else {

        error_set(errp, QERR_SOCKET_CREATE_FAILED);

    }

    qemu_opts_del(opts);

    return sock;

}
