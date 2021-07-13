static CharDriverState *qemu_chr_open_tty_fd(int fd)

{

    CharDriverState *chr;



    tty_serial_init(fd, 115200, 'N', 8, 1);

    chr = qemu_chr_open_fd(fd, fd);

    chr->chr_ioctl = tty_serial_ioctl;

    chr->chr_close = qemu_chr_close_tty;

    return chr;

}
