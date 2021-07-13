static void ics_simple_realize(DeviceState *dev, Error **errp)

{

    ICSState *ics = ICS_SIMPLE(dev);



    if (!ics->nr_irqs) {

        error_setg(errp, "Number of interrupts needs to be greater 0");

        return;

    }

    ics->irqs = g_malloc0(ics->nr_irqs * sizeof(ICSIRQState));

    ics->qirqs = qemu_allocate_irqs(ics_simple_set_irq, ics, ics->nr_irqs);



    qemu_register_reset(ics_simple_reset, dev);

}
