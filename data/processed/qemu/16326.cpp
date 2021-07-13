static void sdhci_initfn(Object *obj)

{

    SDHCIState *s = SDHCI(obj);

    DriveInfo *di;



    di = drive_get_next(IF_SD);

    s->card = sd_init(di ? di->bdrv : NULL, false);

    if (s->card == NULL) {

        exit(1);

    }

    s->eject_cb = qemu_allocate_irqs(sdhci_insert_eject_cb, s, 1)[0];

    s->ro_cb = qemu_allocate_irqs(sdhci_card_readonly_cb, s, 1)[0];

    sd_set_cb(s->card, s->ro_cb, s->eject_cb);



    s->insert_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, sdhci_raise_insertion_irq, s);

    s->transfer_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, sdhci_do_data_transfer, s);

}
