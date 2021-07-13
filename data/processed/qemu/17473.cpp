static int do_token_out(USBDevice *s, USBPacket *p)

{

    assert(p->devep == 0);



    switch(s->setup_state) {

    case SETUP_STATE_ACK:

        if (s->setup_buf[0] & USB_DIR_IN) {

            s->setup_state = SETUP_STATE_IDLE;

            /* transfer OK */

        } else {

            /* ignore additional output */

        }

        return 0;



    case SETUP_STATE_DATA:

        if (!(s->setup_buf[0] & USB_DIR_IN)) {

            int len = s->setup_len - s->setup_index;

            if (len > p->iov.size) {

                len = p->iov.size;

            }

            usb_packet_copy(p, s->data_buf + s->setup_index, len);

            s->setup_index += len;

            if (s->setup_index >= s->setup_len)

                s->setup_state = SETUP_STATE_ACK;

            return len;

        }



        s->setup_state = SETUP_STATE_IDLE;

        return USB_RET_STALL;



    default:

        return USB_RET_STALL;

    }

}
