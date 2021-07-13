iscsi_readcapacity10_cb(struct iscsi_context *iscsi, int status,

                        void *command_data, void *opaque)

{

    struct IscsiTask *itask = opaque;

    struct scsi_readcapacity10 *rc10;

    struct scsi_task *task = command_data;



    if (status != 0) {

        error_report("iSCSI: Failed to read capacity of iSCSI lun. %s",

                     iscsi_get_error(iscsi));

        itask->status   = 1;

        itask->complete = 1;

        scsi_free_scsi_task(task);

        return;

    }



    rc10 = scsi_datain_unmarshall(task);

    if (rc10 == NULL) {

        error_report("iSCSI: Failed to unmarshall readcapacity10 data.");

        itask->status   = 1;

        itask->complete = 1;

        scsi_free_scsi_task(task);

        return;

    }



    itask->iscsilun->block_size = rc10->block_size;

    if (rc10->lba == 0) {

        /* blank disk loaded */

        itask->iscsilun->num_blocks = 0;

    } else {

        itask->iscsilun->num_blocks = rc10->lba + 1;

    }

    itask->bs->total_sectors    = itask->iscsilun->num_blocks *

                               itask->iscsilun->block_size / BDRV_SECTOR_SIZE ;



    itask->status   = 0;

    itask->complete = 1;

    scsi_free_scsi_task(task);

}
