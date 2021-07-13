static void an5206_init(QEMUMachineInitArgs *args)

{

    ram_addr_t ram_size = args->ram_size;

    const char *cpu_model = args->cpu_model;

    const char *kernel_filename = args->kernel_filename;

    CPUM68KState *env;

    int kernel_size;

    uint64_t elf_entry;

    target_phys_addr_t entry;

    MemoryRegion *address_space_mem = get_system_memory();

    MemoryRegion *ram = g_new(MemoryRegion, 1);

    MemoryRegion *sram = g_new(MemoryRegion, 1);



    if (!cpu_model)

        cpu_model = "m5206";

    env = cpu_init(cpu_model);

    if (!env) {

        hw_error("Unable to find m68k CPU definition\n");

    }



    /* Initialize CPU registers.  */

    env->vbr = 0;

    /* TODO: allow changing MBAR and RAMBAR.  */

    env->mbar = AN5206_MBAR_ADDR | 1;

    env->rambar0 = AN5206_RAMBAR_ADDR | 1;



    /* DRAM at address zero */

    memory_region_init_ram(ram, "an5206.ram", ram_size);

    vmstate_register_ram_global(ram);

    memory_region_add_subregion(address_space_mem, 0, ram);



    /* Internal SRAM.  */

    memory_region_init_ram(sram, "an5206.sram", 512);

    vmstate_register_ram_global(sram);

    memory_region_add_subregion(address_space_mem, AN5206_RAMBAR_ADDR, sram);



    mcf5206_init(address_space_mem, AN5206_MBAR_ADDR, env);



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

        kernel_size = load_image_targphys(kernel_filename, KERNEL_LOAD_ADDR,

                                          ram_size - KERNEL_LOAD_ADDR);

        entry = KERNEL_LOAD_ADDR;

    }

    if (kernel_size < 0) {

        fprintf(stderr, "qemu: could not load kernel '%s'\n", kernel_filename);

        exit(1);

    }



    env->pc = entry;

}
