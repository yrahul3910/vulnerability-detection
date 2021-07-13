static void s390_virtio_net_class_init(ObjectClass *klass, void *data)

{

    DeviceClass *dc = DEVICE_CLASS(klass);

    VirtIOS390DeviceClass *k = VIRTIO_S390_DEVICE_CLASS(klass);



    k->init = s390_virtio_net_init;

    dc->props = s390_virtio_net_properties;

    dc->alias = "virtio-net";

}
