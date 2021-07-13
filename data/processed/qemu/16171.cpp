static void scsi_hd_class_initfn(ObjectClass *klass, void *data)

{

    DeviceClass *dc = DEVICE_CLASS(klass);

    SCSIDeviceClass *sc = SCSI_DEVICE_CLASS(klass);



    sc->init         = scsi_hd_initfn;

    sc->destroy      = scsi_destroy;

    sc->alloc_req    = scsi_new_request;

    sc->unit_attention_reported = scsi_disk_unit_attention_reported;

    dc->fw_name = "disk";

    dc->desc = "virtual SCSI disk";

    dc->reset = scsi_disk_reset;

    dc->props = scsi_hd_properties;

    dc->vmsd  = &vmstate_scsi_disk_state;

}
