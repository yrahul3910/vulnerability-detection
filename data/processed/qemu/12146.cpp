static USBDevice *usb_msd_init(const char *filename)

{

    static int nr=0;

    char id[8];

    QemuOpts *opts;

    DriveInfo *dinfo;

    USBDevice *dev;

    int fatal_error;

    const char *p1;

    char fmt[32];



    /* parse -usbdevice disk: syntax into drive opts */

    snprintf(id, sizeof(id), "usb%d", nr++);

    opts = qemu_opts_create(&qemu_drive_opts, id, 0);



    p1 = strchr(filename, ':');

    if (p1++) {

        const char *p2;



        if (strstart(filename, "format=", &p2)) {

            int len = MIN(p1 - p2, sizeof(fmt));

            pstrcpy(fmt, len, p2);

            qemu_opt_set(opts, "format", fmt);

        } else if (*filename != ':') {

            printf("unrecognized USB mass-storage option %s\n", filename);



        filename = p1;


    if (!*filename) {

        printf("block device specification needed\n");



    qemu_opt_set(opts, "file", filename);

    qemu_opt_set(opts, "if", "none");



    /* create host drive */

    dinfo = drive_init(opts, NULL, &fatal_error);

    if (!dinfo) {

        qemu_opts_del(opts);





    /* create guest device */

    dev = usb_create(NULL /* FIXME */, "usb-storage");




    qdev_prop_set_drive(&dev->qdev, "drive", dinfo);

    if (qdev_init(&dev->qdev) < 0)




    return dev;
