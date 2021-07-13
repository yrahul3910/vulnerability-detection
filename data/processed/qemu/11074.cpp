static void lsi_do_command(LSIState *s)

{

    SCSIDevice *dev;

    uint8_t buf[16];

    uint32_t id;

    int n;



    DPRINTF("Send command len=%d\n", s->dbc);

    if (s->dbc > 16)

        s->dbc = 16;

    cpu_physical_memory_read(s->dnad, buf, s->dbc);

    s->sfbr = buf[0];

    s->command_complete = 0;



    id = (s->select_tag >> 8) & 0xf;

    dev = s->bus.devs[id];

    if (!dev) {

        lsi_bad_selection(s, id);

        return;

    }



    assert(s->current == NULL);

    s->current = qemu_mallocz(sizeof(lsi_request));

    s->current->tag = s->select_tag;

    s->current->req = scsi_req_new(dev, s->current->tag, s->current_lun,

                                   s->current);



    n = scsi_req_enqueue(s->current->req, buf);

    if (n) {

        if (n > 0) {

            lsi_set_phase(s, PHASE_DI);

        } else if (n < 0) {

            lsi_set_phase(s, PHASE_DO);

        }

        scsi_req_continue(s->current->req);

    }

    if (!s->command_complete) {

        if (n) {

            /* Command did not complete immediately so disconnect.  */

            lsi_add_msg_byte(s, 2); /* SAVE DATA POINTER */

            lsi_add_msg_byte(s, 4); /* DISCONNECT */

            /* wait data */

            lsi_set_phase(s, PHASE_MI);

            s->msg_action = 1;

            lsi_queue_command(s);

        } else {

            /* wait command complete */

            lsi_set_phase(s, PHASE_DI);

        }

    }

}
