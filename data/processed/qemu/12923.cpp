void mips_r4k_init(MachineState *machine)

{

    ram_addr_t ram_size = machine->ram_size;

    const char *cpu_model = machine->cpu_model;

    const char *kernel_filename = machine->kernel_filename;

    const char *kernel_cmdline = machine->kernel_cmdline;

    const char *initrd_filename = machine->initrd_filename;

    char *filename;

    MemoryRegion *address_space_mem = get_system_memory();

    MemoryRegion *ram = g_new(MemoryRegion, 1);

    MemoryRegion *bios;

    MemoryRegion *iomem = g_new(MemoryRegion, 1);

    MemoryRegion *isa_io = g_new(MemoryRegion, 1);

    MemoryRegion *isa_mem = g_new(MemoryRegion, 1);

    int bios_size;

    MIPSCPU *cpu;

    CPUMIPSState *env;

    ResetData *reset_info;

    int i;

    qemu_irq *i8259;

    ISABus *isa_bus;

    DriveInfo *hd[MAX_IDE_BUS * MAX_IDE_DEVS];

    DriveInfo *dinfo;

    int be;



    /* init CPUs */

    if (cpu_model == NULL) {

#ifdef TARGET_MIPS64

        cpu_model = "R4000";

#else

        cpu_model = "24Kf";

#endif

    }

    cpu = cpu_mips_init(cpu_model);

    if (cpu == NULL) {

        fprintf(stderr, "Unable to find CPU definition\n");

        exit(1);

    }

    env = &cpu->env;



    reset_info = g_malloc0(sizeof(ResetData));

    reset_info->cpu = cpu;

    reset_info->vector = env->active_tc.PC;

    qemu_register_reset(main_cpu_reset, reset_info);



    /* allocate RAM */

    if (ram_size > (256 << 20)) {

        fprintf(stderr,

                "qemu: Too much memory for this machine: %d MB, maximum 256 MB\n",

                ((unsigned int)ram_size / (1 << 20)));

        exit(1);

    }

    memory_region_allocate_system_memory(ram, NULL, "mips_r4k.ram", ram_size);



    memory_region_add_subregion(address_space_mem, 0, ram);



    memory_region_init_io(iomem, NULL, &mips_qemu_ops, NULL, "mips-qemu", 0x10000);

    memory_region_add_subregion(address_space_mem, 0x1fbf0000, iomem);



    /* Try to load a BIOS image. If this fails, we continue regardless,

       but initialize the hardware ourselves. When a kernel gets

       preloaded we also initialize the hardware, since the BIOS wasn't

       run. */

    if (bios_name == NULL)

        bios_name = BIOS_FILENAME;

    filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, bios_name);

    if (filename) {

        bios_size = get_image_size(filename);

    } else {

        bios_size = -1;

    }

#ifdef TARGET_WORDS_BIGENDIAN

    be = 1;

#else

    be = 0;

#endif

    if ((bios_size > 0) && (bios_size <= BIOS_SIZE)) {

        bios = g_new(MemoryRegion, 1);

        memory_region_init_ram(bios, NULL, "mips_r4k.bios", BIOS_SIZE,

                               &error_abort);

        vmstate_register_ram_global(bios);

        memory_region_set_readonly(bios, true);

        memory_region_add_subregion(get_system_memory(), 0x1fc00000, bios);



        load_image_targphys(filename, 0x1fc00000, BIOS_SIZE);

    } else if ((dinfo = drive_get(IF_PFLASH, 0, 0)) != NULL) {

        uint32_t mips_rom = 0x00400000;

        if (!pflash_cfi01_register(0x1fc00000, NULL, "mips_r4k.bios", mips_rom,

                                   blk_by_legacy_dinfo(dinfo),

                                   sector_len, mips_rom / sector_len,

                                   4, 0, 0, 0, 0, be)) {

            fprintf(stderr, "qemu: Error registering flash memory.\n");

	}

    } else if (!qtest_enabled()) {

	/* not fatal */

        fprintf(stderr, "qemu: Warning, could not load MIPS bios '%s'\n",

		bios_name);

    }

    g_free(filename);



    if (kernel_filename) {

        loaderparams.ram_size = ram_size;

        loaderparams.kernel_filename = kernel_filename;

        loaderparams.kernel_cmdline = kernel_cmdline;

        loaderparams.initrd_filename = initrd_filename;

        reset_info->vector = load_kernel();

    }



    /* Init CPU internal devices */

    cpu_mips_irq_init_cpu(env);

    cpu_mips_clock_init(env);



    /* ISA bus: IO space at 0x14000000, mem space at 0x10000000 */

    memory_region_init_alias(isa_io, NULL, "isa-io",

                             get_system_io(), 0, 0x00010000);

    memory_region_init(isa_mem, NULL, "isa-mem", 0x01000000);

    memory_region_add_subregion(get_system_memory(), 0x14000000, isa_io);

    memory_region_add_subregion(get_system_memory(), 0x10000000, isa_mem);

    isa_bus = isa_bus_new(NULL, isa_mem, get_system_io());



    /* The PIC is attached to the MIPS CPU INT0 pin */

    i8259 = i8259_init(isa_bus, env->irq[2]);

    isa_bus_irqs(isa_bus, i8259);



    rtc_init(isa_bus, 2000, NULL);



    pit = pit_init(isa_bus, 0x40, 0, NULL);



    serial_hds_isa_init(isa_bus, MAX_SERIAL_PORTS);



    isa_vga_init(isa_bus);



    if (nd_table[0].used)

        isa_ne2000_init(isa_bus, 0x300, 9, &nd_table[0]);



    ide_drive_get(hd, ARRAY_SIZE(hd));

    for(i = 0; i < MAX_IDE_BUS; i++)

        isa_ide_init(isa_bus, ide_iobase[i], ide_iobase2[i], ide_irq[i],

                     hd[MAX_IDE_DEVS * i],

		     hd[MAX_IDE_DEVS * i + 1]);



    isa_create_simple(isa_bus, "i8042");

}
