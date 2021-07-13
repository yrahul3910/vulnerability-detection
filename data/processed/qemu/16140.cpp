static int get_uint8_equal(QEMUFile *f, void *pv, size_t size)

{

    uint8_t *v = pv;

    uint8_t v2;

    qemu_get_8s(f, &v2);



    if (*v == v2) {

        return 0;

    }

    return -EINVAL;

}
