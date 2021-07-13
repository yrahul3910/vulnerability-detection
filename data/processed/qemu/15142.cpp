QEMUFile *qemu_fopen_ops(void *opaque, const QEMUFileOps *ops)

{

    QEMUFile *f;



    f = g_malloc0(sizeof(QEMUFile));



    f->opaque = opaque;

    f->ops = ops;

    f->is_write = 0;



    return f;

}
