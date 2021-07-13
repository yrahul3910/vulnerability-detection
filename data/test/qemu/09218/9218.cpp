static int ccid_handle_data(USBDevice *dev, USBPacket *p)

{

    USBCCIDState *s = DO_UPCAST(USBCCIDState, dev, dev);

    int ret = 0;

    uint8_t buf[2];



    switch (p->pid) {

    case USB_TOKEN_OUT:

        ret = ccid_handle_bulk_out(s, p);

        break;



    case USB_TOKEN_IN:

        switch (p->devep & 0xf) {

        case CCID_BULK_IN_EP:

            if (!p->iov.size) {

                ret = USB_RET_NAK;

            } else {

                ret = ccid_bulk_in_copy_to_guest(s, p);

            }

            break;

        case CCID_INT_IN_EP:

            if (s->notify_slot_change) {

                /* page 56, RDR_to_PC_NotifySlotChange */

                buf[0] = CCID_MESSAGE_TYPE_RDR_to_PC_NotifySlotChange;

                buf[1] = s->bmSlotICCState;

                usb_packet_copy(p, buf, 2);

                ret = 2;

                s->notify_slot_change = false;

                s->bmSlotICCState &= ~SLOT_0_CHANGED_MASK;

                DPRINTF(s, D_INFO,

                        "handle_data: int_in: notify_slot_change %X, "

                        "requested len %zd\n",

                        s->bmSlotICCState, p->iov.size);

            }

            break;

        default:

            DPRINTF(s, 1, "Bad endpoint\n");

            ret = USB_RET_STALL;

            break;

        }

        break;

    default:

        DPRINTF(s, 1, "Bad token\n");

        ret = USB_RET_STALL;

        break;

    }



    return ret;

}
