static uint64_t virtio_pci_config_read(void *opaque, hwaddr addr,

                                       unsigned size)

{

    VirtIOPCIProxy *proxy = opaque;

    uint32_t config = VIRTIO_PCI_CONFIG(&proxy->pci_dev);

    uint64_t val = 0;

    if (addr < config) {

        return virtio_ioport_read(proxy, addr);

    }

    addr -= config;



    switch (size) {

    case 1:

        val = virtio_config_readb(proxy->vdev, addr);

        break;

    case 2:

        val = virtio_config_readw(proxy->vdev, addr);

        if (virtio_is_big_endian()) {

            val = bswap16(val);

        }

        break;

    case 4:

        val = virtio_config_readl(proxy->vdev, addr);

        if (virtio_is_big_endian()) {

            val = bswap32(val);

        }

        break;

    }

    return val;

}
