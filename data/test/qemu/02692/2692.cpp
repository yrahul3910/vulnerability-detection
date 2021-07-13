static void mcf5208evb_init(QEMUMachineInitArgs *args)

{

    ram_addr_t ram_size = args->ram_size;

    const char *cpu_model = args->cpu_model;

    const char *kernel_filename = args->kernel_filename;

    CPUM68KState *env;

    int kernel_size;

    uint64_t elf_entry;

    target_phys_addr_t entry;

    qemu_irq *pic;

    MemoryRegion *address_space_mem = get_system_memory();

    MemoryRegion *ram = g_new(MemoryRegion, 1);

    MemoryRegion *sram = g_new(MemoryRegion, 1);



    if (!cpu_model)

        cpu_model = "m5208";

    env = cpu_init(cpu_model);

    if (!env) {

        fprintf(stderr, "Unable to find m68k CPU definition\n");

        exit(1);

    }



    /* Initialize CPU registers.  */

    env->vbr = 0;

    /* TODO: Configure BARs.  */



    /* DRAM at 0x40000000 */

    memory_region_init_ram(ram, "mcf5208.ram", ram_size);

    vmstate_register_ram_global(ram);

    memory_region_add_subregion(address_space_mem, 0x40000000, ram);



    /* Internal SRAM.  */

    memory_region_init_ram(sram, "mcf5208.sram", 16384);

    vmstate_register_ram_global(sram);

    memory_region_add_subregion(address_space_mem, 0x80000000, sram);



    /* Internal peripherals.  */

    pic = mcf_intc_init(address_space_mem, 0xfc048000, env);



    mcf_uart_mm_init(address_space_mem, 0xfc060000, pic[26], serial_hds[0]);

    mcf_uart_mm_init(address_space_mem, 0xfc064000, pic[27], serial_hds[1]);

    mcf_uart_mm_init(address_space_mem, 0xfc068000, pic[28], serial_hds[2]);



    mcf5208_sys_init(address_space_mem, pic);



    if (nb_nics > 1) {

        fprintf(stderr, "Too many NICs\n");

        exit(1);

    }

    if (nd_table[0].used)

        mcf_fec_init(address_space_mem, &nd_table[0],

                     0xfc030000, pic + 36);



    /*  0xfc000000 SCM.  */

    /*  0xfc004000 XBS.  */

    /*  0xfc008000 FlexBus CS.  */

    /* 0xfc030000 FEC.  */

    /*  0xfc040000 SCM + Power management.  */

    /*  0xfc044000 eDMA.  */

    /* 0xfc048000 INTC.  */

    /*  0xfc058000 I2C.  */

    /*  0xfc05c000 QSPI.  */

    /* 0xfc060000 UART0.  */

    /* 0xfc064000 UART0.  */

    /* 0xfc068000 UART0.  */

    /*  0xfc070000 DMA timers.  */

    /* 0xfc080000 PIT0.  */

    /* 0xfc084000 PIT1.  */

    /*  0xfc088000 EPORT.  */

    /*  0xfc08c000 Watchdog.  */

    /*  0xfc090000 clock module.  */

    /*  0xfc0a0000 CCM + reset.  */

    /*  0xfc0a4000 GPIO.  */

    /* 0xfc0a8000 SDRAM controller.  */



    /* Load kernel.  */

    if (!kernel_filename) {

        fprintf(stderr, "Kernel image must be specified\n");

        exit(1);

    }



    kernel_size = load_elf(kernel_filename, NULL, NULL, &elf_entry,

                           NULL, NULL, 1, ELF_MACHINE, 0);

    entry = elf_entry;

    if (kernel_size < 0) {

        kernel_size = load_uimage(kernel_filename, &entry, NULL, NULL);

    }

    if (kernel_size < 0) {

        kernel_size = load_image_targphys(kernel_filename, 0x40000000,

                                          ram_size);

        entry = 0x40000000;

    }

    if (kernel_size < 0) {

        fprintf(stderr, "qemu: could not load kernel '%s'\n", kernel_filename);

        exit(1);

    }



    env->pc = entry;

}
