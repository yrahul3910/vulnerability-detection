static int ehci_execute(EHCIQueue *q)

{

    USBDevice *dev;

    int ret;

    int endp;

    int devadr;



    if ( !(q->qh.token & QTD_TOKEN_ACTIVE)) {

        fprintf(stderr, "Attempting to execute inactive QH\n");

        return USB_RET_PROCERR;

    }



    q->tbytes = (q->qh.token & QTD_TOKEN_TBYTES_MASK) >> QTD_TOKEN_TBYTES_SH;

    if (q->tbytes > BUFF_SIZE) {

        fprintf(stderr, "Request for more bytes than allowed\n");

        return USB_RET_PROCERR;

    }



    q->pid = (q->qh.token & QTD_TOKEN_PID_MASK) >> QTD_TOKEN_PID_SH;

    switch(q->pid) {

        case 0: q->pid = USB_TOKEN_OUT; break;

        case 1: q->pid = USB_TOKEN_IN; break;

        case 2: q->pid = USB_TOKEN_SETUP; break;

        default: fprintf(stderr, "bad token\n"); break;

    }



    if (ehci_init_transfer(q) != 0) {

        return USB_RET_PROCERR;

    }



    endp = get_field(q->qh.epchar, QH_EPCHAR_EP);

    devadr = get_field(q->qh.epchar, QH_EPCHAR_DEVADDR);



    ret = USB_RET_NODEV;



    usb_packet_setup(&q->packet, q->pid, devadr, endp);

    usb_packet_map(&q->packet, &q->sgl);



    // TO-DO: associating device with ehci port

    dev = ehci_find_device(q->ehci, q->packet.devaddr);

    ret = usb_handle_packet(dev, &q->packet);

    DPRINTF("submit: qh %x next %x qtd %x pid %x len %zd "

            "(total %d) endp %x ret %d\n",

            q->qhaddr, q->qh.next, q->qtdaddr, q->pid,

            q->packet.iov.size, q->tbytes, endp, ret);



    if (ret > BUFF_SIZE) {

        fprintf(stderr, "ret from usb_handle_packet > BUFF_SIZE\n");

        return USB_RET_PROCERR;

    }



    return ret;

}
