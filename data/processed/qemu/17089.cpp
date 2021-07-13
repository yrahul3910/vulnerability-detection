FWCfgState *pc_memory_init(PCMachineState *pcms,

                           MemoryRegion *system_memory,

                           MemoryRegion *rom_memory,

                           MemoryRegion **ram_memory,

                           PcGuestInfo *guest_info)

{

    int linux_boot, i;

    MemoryRegion *ram, *option_rom_mr;

    MemoryRegion *ram_below_4g, *ram_above_4g;

    FWCfgState *fw_cfg;

    MachineState *machine = MACHINE(pcms);



    assert(machine->ram_size == pcms->below_4g_mem_size +

                                pcms->above_4g_mem_size);



    linux_boot = (machine->kernel_filename != NULL);



    /* Allocate RAM.  We allocate it as a single memory region and use

     * aliases to address portions of it, mostly for backwards compatibility

     * with older qemus that used qemu_ram_alloc().

     */

    ram = g_malloc(sizeof(*ram));

    memory_region_allocate_system_memory(ram, NULL, "pc.ram",

                                         machine->ram_size);

    *ram_memory = ram;

    ram_below_4g = g_malloc(sizeof(*ram_below_4g));

    memory_region_init_alias(ram_below_4g, NULL, "ram-below-4g", ram,

                             0, pcms->below_4g_mem_size);

    memory_region_add_subregion(system_memory, 0, ram_below_4g);

    e820_add_entry(0, pcms->below_4g_mem_size, E820_RAM);

    if (pcms->above_4g_mem_size > 0) {

        ram_above_4g = g_malloc(sizeof(*ram_above_4g));

        memory_region_init_alias(ram_above_4g, NULL, "ram-above-4g", ram,

                                 pcms->below_4g_mem_size,

                                 pcms->above_4g_mem_size);

        memory_region_add_subregion(system_memory, 0x100000000ULL,

                                    ram_above_4g);

        e820_add_entry(0x100000000ULL, pcms->above_4g_mem_size, E820_RAM);

    }



    if (!guest_info->has_reserved_memory &&

        (machine->ram_slots ||

         (machine->maxram_size > machine->ram_size))) {

        MachineClass *mc = MACHINE_GET_CLASS(machine);



        error_report("\"-memory 'slots|maxmem'\" is not supported by: %s",

                     mc->name);

        exit(EXIT_FAILURE);

    }



    /* initialize hotplug memory address space */

    if (guest_info->has_reserved_memory &&

        (machine->ram_size < machine->maxram_size)) {

        ram_addr_t hotplug_mem_size =

            machine->maxram_size - machine->ram_size;



        if (machine->ram_slots > ACPI_MAX_RAM_SLOTS) {

            error_report("unsupported amount of memory slots: %"PRIu64,

                         machine->ram_slots);

            exit(EXIT_FAILURE);

        }



        if (QEMU_ALIGN_UP(machine->maxram_size,

                          TARGET_PAGE_SIZE) != machine->maxram_size) {

            error_report("maximum memory size must by aligned to multiple of "

                         "%d bytes", TARGET_PAGE_SIZE);

            exit(EXIT_FAILURE);

        }



        pcms->hotplug_memory.base =

            ROUND_UP(0x100000000ULL + pcms->above_4g_mem_size, 1ULL << 30);



        if (pcms->enforce_aligned_dimm) {

            /* size hotplug region assuming 1G page max alignment per slot */

            hotplug_mem_size += (1ULL << 30) * machine->ram_slots;

        }



        if ((pcms->hotplug_memory.base + hotplug_mem_size) <

            hotplug_mem_size) {

            error_report("unsupported amount of maximum memory: " RAM_ADDR_FMT,

                         machine->maxram_size);

            exit(EXIT_FAILURE);

        }



        memory_region_init(&pcms->hotplug_memory.mr, OBJECT(pcms),

                           "hotplug-memory", hotplug_mem_size);

        memory_region_add_subregion(system_memory, pcms->hotplug_memory.base,

                                    &pcms->hotplug_memory.mr);

    }



    /* Initialize PC system firmware */

    pc_system_firmware_init(rom_memory, guest_info->isapc_ram_fw);



    option_rom_mr = g_malloc(sizeof(*option_rom_mr));

    memory_region_init_ram(option_rom_mr, NULL, "pc.rom", PC_ROM_SIZE,

                           &error_abort);

    vmstate_register_ram_global(option_rom_mr);

    memory_region_add_subregion_overlap(rom_memory,

                                        PC_ROM_MIN_VGA,

                                        option_rom_mr,

                                        1);



    fw_cfg = bochs_bios_init();

    rom_set_fw(fw_cfg);



    if (guest_info->has_reserved_memory && pcms->hotplug_memory.base) {

        uint64_t *val = g_malloc(sizeof(*val));

        PCMachineClass *pcmc = PC_MACHINE_GET_CLASS(pcms);

        uint64_t res_mem_end = pcms->hotplug_memory.base;



        if (!pcmc->broken_reserved_end) {

            res_mem_end += memory_region_size(&pcms->hotplug_memory.mr);

        }

        *val = cpu_to_le64(ROUND_UP(res_mem_end, 0x1ULL << 30));

        fw_cfg_add_file(fw_cfg, "etc/reserved-memory-end", val, sizeof(*val));

    }



    if (linux_boot) {

        load_linux(pcms, fw_cfg);

    }



    for (i = 0; i < nb_option_roms; i++) {

        rom_add_option(option_rom[i].name, option_rom[i].bootindex);

    }

    guest_info->fw_cfg = fw_cfg;

    return fw_cfg;

}
