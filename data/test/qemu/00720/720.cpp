static int debugcon_parse(const char *devname)

{

    QemuOpts *opts;



    if (!qemu_chr_new("debugcon", devname, NULL)) {

        exit(1);

    }

    opts = qemu_opts_create(qemu_find_opts("device"), "debugcon", 1, NULL);

    if (!opts) {

        fprintf(stderr, "qemu: already have a debugcon device\n");

        exit(1);

    }

    qemu_opt_set(opts, "driver", "isa-debugcon", &error_abort);

    qemu_opt_set(opts, "chardev", "debugcon", &error_abort);

    return 0;

}
