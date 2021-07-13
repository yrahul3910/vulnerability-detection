static void uhci_ioport_writew(void *opaque, uint32_t addr, uint32_t val)

{

    UHCIState *s = opaque;



    addr &= 0x1f;

    trace_usb_uhci_mmio_writew(addr, val);



    switch(addr) {

    case 0x00:

        if ((val & UHCI_CMD_RS) && !(s->cmd & UHCI_CMD_RS)) {

            /* start frame processing */

            trace_usb_uhci_schedule_start();

            s->expire_time = qemu_get_clock_ns(vm_clock) +

                (get_ticks_per_sec() / FRAME_TIMER_FREQ);

            qemu_mod_timer(s->frame_timer, qemu_get_clock_ns(vm_clock));

            s->status &= ~UHCI_STS_HCHALTED;

        } else if (!(val & UHCI_CMD_RS)) {

            s->status |= UHCI_STS_HCHALTED;

        }

        if (val & UHCI_CMD_GRESET) {

            UHCIPort *port;

            int i;



            /* send reset on the USB bus */

            for(i = 0; i < NB_PORTS; i++) {

                port = &s->ports[i];

                usb_device_reset(port->port.dev);

            }

            uhci_reset(s);

            return;

        }

        if (val & UHCI_CMD_HCRESET) {

            uhci_reset(s);

            return;

        }

        s->cmd = val;

        break;

    case 0x02:

        s->status &= ~val;

        /* XXX: the chip spec is not coherent, so we add a hidden

           register to distinguish between IOC and SPD */

        if (val & UHCI_STS_USBINT)

            s->status2 = 0;

        uhci_update_irq(s);

        break;

    case 0x04:

        s->intr = val;

        uhci_update_irq(s);

        break;

    case 0x06:

        if (s->status & UHCI_STS_HCHALTED)

            s->frnum = val & 0x7ff;

        break;

    case 0x10 ... 0x1f:

        {

            UHCIPort *port;

            USBDevice *dev;

            int n;



            n = (addr >> 1) & 7;

            if (n >= NB_PORTS)

                return;

            port = &s->ports[n];

            dev = port->port.dev;

            if (dev && dev->attached) {

                /* port reset */

                if ( (val & UHCI_PORT_RESET) &&

                     !(port->ctrl & UHCI_PORT_RESET) ) {

                    usb_device_reset(dev);

                }

            }

            port->ctrl &= UHCI_PORT_READ_ONLY;

            /* enabled may only be set if a device is connected */

            if (!(port->ctrl & UHCI_PORT_CCS)) {

                val &= ~UHCI_PORT_EN;

            }

            port->ctrl |= (val & ~UHCI_PORT_READ_ONLY);

            /* some bits are reset when a '1' is written to them */

            port->ctrl &= ~(val & UHCI_PORT_WRITE_CLEAR);

        }

        break;

    }

}
