static int vhost_virtqueue_start(struct vhost_dev *dev,

                                struct VirtIODevice *vdev,

                                struct vhost_virtqueue *vq,

                                unsigned idx)

{

    hwaddr s, l, a;

    int r;

    int vhost_vq_index = idx - dev->vq_index;

    struct vhost_vring_file file = {

        .index = vhost_vq_index

    };

    struct vhost_vring_state state = {

        .index = vhost_vq_index

    };

    struct VirtQueue *vvq = virtio_get_queue(vdev, idx);



    assert(idx >= dev->vq_index && idx < dev->vq_index + dev->nvqs);



    vq->num = state.num = virtio_queue_get_num(vdev, idx);

    r = dev->vhost_ops->vhost_call(dev, VHOST_SET_VRING_NUM, &state);

    if (r) {

        return -errno;

    }



    state.num = virtio_queue_get_last_avail_idx(vdev, idx);

    r = dev->vhost_ops->vhost_call(dev, VHOST_SET_VRING_BASE, &state);

    if (r) {

        return -errno;

    }



    if (!virtio_has_feature(vdev, VIRTIO_F_VERSION_1) &&

        virtio_legacy_is_cross_endian(vdev)) {

        r = vhost_virtqueue_set_vring_endian_legacy(dev,

                                                    virtio_is_big_endian(vdev),

                                                    vhost_vq_index);

        if (r) {

            return -errno;

        }

    }



    s = l = virtio_queue_get_desc_size(vdev, idx);

    a = virtio_queue_get_desc_addr(vdev, idx);

    vq->desc = cpu_physical_memory_map(a, &l, 0);

    if (!vq->desc || l != s) {

        r = -ENOMEM;

        goto fail_alloc_desc;

    }

    s = l = virtio_queue_get_avail_size(vdev, idx);

    a = virtio_queue_get_avail_addr(vdev, idx);

    vq->avail = cpu_physical_memory_map(a, &l, 0);

    if (!vq->avail || l != s) {

        r = -ENOMEM;

        goto fail_alloc_avail;

    }

    vq->used_size = s = l = virtio_queue_get_used_size(vdev, idx);

    vq->used_phys = a = virtio_queue_get_used_addr(vdev, idx);

    vq->used = cpu_physical_memory_map(a, &l, 1);

    if (!vq->used || l != s) {

        r = -ENOMEM;

        goto fail_alloc_used;

    }



    vq->ring_size = s = l = virtio_queue_get_ring_size(vdev, idx);

    vq->ring_phys = a = virtio_queue_get_ring_addr(vdev, idx);

    vq->ring = cpu_physical_memory_map(a, &l, 1);

    if (!vq->ring || l != s) {

        r = -ENOMEM;

        goto fail_alloc_ring;

    }



    r = vhost_virtqueue_set_addr(dev, vq, vhost_vq_index, dev->log_enabled);

    if (r < 0) {

        r = -errno;

        goto fail_alloc;

    }



    file.fd = event_notifier_get_fd(virtio_queue_get_host_notifier(vvq));

    r = dev->vhost_ops->vhost_call(dev, VHOST_SET_VRING_KICK, &file);

    if (r) {

        r = -errno;

        goto fail_kick;

    }



    /* Clear and discard previous events if any. */

    event_notifier_test_and_clear(&vq->masked_notifier);



    return 0;



fail_kick:

fail_alloc:

    cpu_physical_memory_unmap(vq->ring, virtio_queue_get_ring_size(vdev, idx),

                              0, 0);

fail_alloc_ring:

    cpu_physical_memory_unmap(vq->used, virtio_queue_get_used_size(vdev, idx),

                              0, 0);

fail_alloc_used:

    cpu_physical_memory_unmap(vq->avail, virtio_queue_get_avail_size(vdev, idx),

                              0, 0);

fail_alloc_avail:

    cpu_physical_memory_unmap(vq->desc, virtio_queue_get_desc_size(vdev, idx),

                              0, 0);

fail_alloc_desc:

    return r;

}
