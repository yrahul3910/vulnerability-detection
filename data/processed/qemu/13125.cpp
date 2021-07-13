void virtio_queue_notify_vq(VirtQueue *vq)

{

    if (vq->vring.desc) {

        VirtIODevice *vdev = vq->vdev;

        trace_virtio_queue_notify(vdev, vq - vdev->vq, vq);

        vq->handle_output(vdev, vq);

    }

}
