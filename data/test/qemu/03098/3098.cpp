static void mps2_common_init(MachineState *machine)

{

    MPS2MachineState *mms = MPS2_MACHINE(machine);

    MPS2MachineClass *mmc = MPS2_MACHINE_GET_CLASS(machine);

    MemoryRegion *system_memory = get_system_memory();

    DeviceState *armv7m, *sccdev;



    if (!machine->cpu_model) {

        machine->cpu_model = mmc->cpu_model;

    }



    if (strcmp(machine->cpu_model, mmc->cpu_model) != 0) {

        error_report("This board can only be used with CPU %s", mmc->cpu_model);

        exit(1);

    }



    /* The FPGA images have an odd combination of different RAMs,

     * because in hardware they are different implementations and

     * connected to different buses, giving varying performance/size

     * tradeoffs. For QEMU they're all just RAM, though. We arbitrarily

     * call the 16MB our "system memory", as it's the largest lump.

     *

     * Common to both boards:

     *  0x21000000..0x21ffffff : PSRAM (16MB)

     * AN385 only:

     *  0x00000000 .. 0x003fffff : ZBT SSRAM1

     *  0x00400000 .. 0x007fffff : mirror of ZBT SSRAM1

     *  0x20000000 .. 0x203fffff : ZBT SSRAM 2&3

     *  0x20400000 .. 0x207fffff : mirror of ZBT SSRAM 2&3

     *  0x01000000 .. 0x01003fff : block RAM (16K)

     *  0x01004000 .. 0x01007fff : mirror of above

     *  0x01008000 .. 0x0100bfff : mirror of above

     *  0x0100c000 .. 0x0100ffff : mirror of above

     * AN511 only:

     *  0x00000000 .. 0x0003ffff : FPGA block RAM

     *  0x00400000 .. 0x007fffff : ZBT SSRAM1

     *  0x20000000 .. 0x2001ffff : SRAM

     *  0x20400000 .. 0x207fffff : ZBT SSRAM 2&3

     *

     * The AN385 has a feature where the lowest 16K can be mapped

     * either to the bottom of the ZBT SSRAM1 or to the block RAM.

     * This is of no use for QEMU so we don't implement it (as if

     * zbt_boot_ctrl is always zero).

     */

    memory_region_allocate_system_memory(&mms->psram,

                                         NULL, "mps.ram", 0x1000000);

    memory_region_add_subregion(system_memory, 0x21000000, &mms->psram);



    switch (mmc->fpga_type) {

    case FPGA_AN385:

        make_ram(&mms->ssram1, "mps.ssram1", 0x0, 0x400000);

        make_ram_alias(&mms->ssram1_m, "mps.ssram1_m", &mms->ssram1, 0x400000);

        make_ram(&mms->ssram23, "mps.ssram23", 0x20000000, 0x400000);

        make_ram_alias(&mms->ssram23_m, "mps.ssram23_m",

                       &mms->ssram23, 0x20400000);

        make_ram(&mms->blockram, "mps.blockram", 0x01000000, 0x4000);

        make_ram_alias(&mms->blockram_m1, "mps.blockram_m1",

                       &mms->blockram, 0x01004000);

        make_ram_alias(&mms->blockram_m2, "mps.blockram_m2",

                       &mms->blockram, 0x01008000);

        make_ram_alias(&mms->blockram_m3, "mps.blockram_m3",

                       &mms->blockram, 0x0100c000);

        break;

    case FPGA_AN511:

        make_ram(&mms->blockram, "mps.blockram", 0x0, 0x40000);

        make_ram(&mms->ssram1, "mps.ssram1", 0x00400000, 0x00800000);

        make_ram(&mms->sram, "mps.sram", 0x20000000, 0x20000);

        make_ram(&mms->ssram23, "mps.ssram23", 0x20400000, 0x400000);

        break;

    default:

        g_assert_not_reached();

    }



    object_initialize(&mms->armv7m, sizeof(mms->armv7m), TYPE_ARMV7M);

    armv7m = DEVICE(&mms->armv7m);

    qdev_set_parent_bus(armv7m, sysbus_get_default());

    switch (mmc->fpga_type) {

    case FPGA_AN385:

        qdev_prop_set_uint32(armv7m, "num-irq", 32);

        break;

    case FPGA_AN511:

        qdev_prop_set_uint32(armv7m, "num-irq", 64);

        break;

    default:

        g_assert_not_reached();

    }

    qdev_prop_set_string(armv7m, "cpu-model", machine->cpu_model);

    object_property_set_link(OBJECT(&mms->armv7m), OBJECT(system_memory),

                             "memory", &error_abort);

    object_property_set_bool(OBJECT(&mms->armv7m), true, "realized",

                             &error_fatal);



    create_unimplemented_device("zbtsmram mirror", 0x00400000, 0x00400000);

    create_unimplemented_device("RESERVED 1", 0x00800000, 0x00800000);

    create_unimplemented_device("Block RAM", 0x01000000, 0x00010000);

    create_unimplemented_device("RESERVED 2", 0x01010000, 0x1EFF0000);

    create_unimplemented_device("RESERVED 3", 0x20800000, 0x00800000);

    create_unimplemented_device("PSRAM", 0x21000000, 0x01000000);

    /* These three ranges all cover multiple devices; we may implement

     * some of them below (in which case the real device takes precedence

     * over the unimplemented-region mapping).

     */

    create_unimplemented_device("CMSDK APB peripheral region @0x40000000",

                                0x40000000, 0x00010000);

    create_unimplemented_device("CMSDK peripheral region @0x40010000",

                                0x40010000, 0x00010000);

    create_unimplemented_device("Extra peripheral region @0x40020000",

                                0x40020000, 0x00010000);

    create_unimplemented_device("RESERVED 4", 0x40030000, 0x001D0000);

    create_unimplemented_device("VGA", 0x41000000, 0x0200000);



    switch (mmc->fpga_type) {

    case FPGA_AN385:

    {

        /* The overflow IRQs for UARTs 0, 1 and 2 are ORed together.

         * Overflow for UARTs 4 and 5 doesn't trigger any interrupt.

         */

        Object *orgate;

        DeviceState *orgate_dev;

        int i;



        orgate = object_new(TYPE_OR_IRQ);

        object_property_set_int(orgate, 6, "num-lines", &error_fatal);

        object_property_set_bool(orgate, true, "realized", &error_fatal);

        orgate_dev = DEVICE(orgate);

        qdev_connect_gpio_out(orgate_dev, 0, qdev_get_gpio_in(armv7m, 12));



        for (i = 0; i < 5; i++) {

            static const hwaddr uartbase[] = {0x40004000, 0x40005000,

                                              0x40006000, 0x40007000,

                                              0x40009000};

            Chardev *uartchr = i < MAX_SERIAL_PORTS ? serial_hds[i] : NULL;

            /* RX irq number; TX irq is always one greater */

            static const int uartirq[] = {0, 2, 4, 18, 20};

            qemu_irq txovrint = NULL, rxovrint = NULL;



            if (i < 3) {

                txovrint = qdev_get_gpio_in(orgate_dev, i * 2);

                rxovrint = qdev_get_gpio_in(orgate_dev, i * 2 + 1);

            }



            cmsdk_apb_uart_create(uartbase[i],

                                  qdev_get_gpio_in(armv7m, uartirq[i] + 1),

                                  qdev_get_gpio_in(armv7m, uartirq[i]),

                                  txovrint, rxovrint,

                                  NULL,

                                  uartchr, SYSCLK_FRQ);

        }

        break;

    }

    case FPGA_AN511:

    {

        /* The overflow IRQs for all UARTs are ORed together.

         * Tx and Rx IRQs for each UART are ORed together.

         */

        Object *orgate;

        DeviceState *orgate_dev;

        int i;



        orgate = object_new(TYPE_OR_IRQ);

        object_property_set_int(orgate, 10, "num-lines", &error_fatal);

        object_property_set_bool(orgate, true, "realized", &error_fatal);

        orgate_dev = DEVICE(orgate);

        qdev_connect_gpio_out(orgate_dev, 0, qdev_get_gpio_in(armv7m, 12));



        for (i = 0; i < 5; i++) {

            /* system irq numbers for the combined tx/rx for each UART */

            static const int uart_txrx_irqno[] = {0, 2, 45, 46, 56};

            static const hwaddr uartbase[] = {0x40004000, 0x40005000,

                                              0x4002c000, 0x4002d000,

                                              0x4002e000};

            Chardev *uartchr = i < MAX_SERIAL_PORTS ? serial_hds[i] : NULL;

            Object *txrx_orgate;

            DeviceState *txrx_orgate_dev;



            txrx_orgate = object_new(TYPE_OR_IRQ);

            object_property_set_int(txrx_orgate, 2, "num-lines", &error_fatal);

            object_property_set_bool(txrx_orgate, true, "realized",

                                     &error_fatal);

            txrx_orgate_dev = DEVICE(txrx_orgate);

            qdev_connect_gpio_out(txrx_orgate_dev, 0,

                                  qdev_get_gpio_in(armv7m, uart_txrx_irqno[i]));

            cmsdk_apb_uart_create(uartbase[i],

                                  qdev_get_gpio_in(txrx_orgate_dev, 0),

                                  qdev_get_gpio_in(txrx_orgate_dev, 1),

                                  qdev_get_gpio_in(orgate_dev, 0),

                                  qdev_get_gpio_in(orgate_dev, 1),

                                  NULL,

                                  uartchr, SYSCLK_FRQ);

        }

        break;

    }

    default:

        g_assert_not_reached();

    }



    cmsdk_apb_timer_create(0x40000000, qdev_get_gpio_in(armv7m, 8), SYSCLK_FRQ);

    cmsdk_apb_timer_create(0x40001000, qdev_get_gpio_in(armv7m, 9), SYSCLK_FRQ);



    object_initialize(&mms->scc, sizeof(mms->scc), TYPE_MPS2_SCC);

    sccdev = DEVICE(&mms->scc);

    qdev_set_parent_bus(sccdev, sysbus_get_default());

    qdev_prop_set_uint32(sccdev, "scc-cfg4", 0x2);

    qdev_prop_set_uint32(sccdev, "scc-aid", 0x02000008);

    qdev_prop_set_uint32(sccdev, "scc-id", mmc->scc_id);

    object_property_set_bool(OBJECT(&mms->scc), true, "realized",

                             &error_fatal);

    sysbus_mmio_map(SYS_BUS_DEVICE(sccdev), 0, 0x4002f000);



    /* In hardware this is a LAN9220; the LAN9118 is software compatible

     * except that it doesn't support the checksum-offload feature.

     */

    lan9118_init(&nd_table[0], 0x40200000,

                 qdev_get_gpio_in(armv7m,

                                  mmc->fpga_type == FPGA_AN385 ? 13 : 47));



    system_clock_scale = NANOSECONDS_PER_SECOND / SYSCLK_FRQ;



    armv7m_load_kernel(ARM_CPU(first_cpu), machine->kernel_filename,

                       0x400000);

}
