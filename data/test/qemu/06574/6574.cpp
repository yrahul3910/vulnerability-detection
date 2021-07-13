void process_incoming_migration(QEMUFile *f)

{

    if (qemu_loadvm_state(f) < 0) {

        fprintf(stderr, "load of migration failed\n");

        exit(0);

    }

    qemu_announce_self();

    DPRINTF("successfully loaded vm state\n");



    incoming_expected = false;



    if (autostart) {

        vm_start();

    } else {

        runstate_set(RSTATE_PRE_LAUNCH);

    }

}
