void ppce500_init(QEMUMachineInitArgs *args, PPCE500Params *params)

{

    MemoryRegion *address_space_mem = get_system_memory();

    MemoryRegion *ram = g_new(MemoryRegion, 1);

    PCIBus *pci_bus;

    CPUPPCState *env = NULL;

    uint64_t elf_entry;

    uint64_t elf_lowaddr;

    hwaddr entry=0;

    hwaddr loadaddr=UIMAGE_LOAD_BASE;

    target_long kernel_size=0;

    target_ulong dt_base = 0;

    target_ulong initrd_base = 0;

    target_long initrd_size = 0;

    target_ulong cur_base = 0;

    int i;

    unsigned int pci_irq_nrs[4] = {1, 2, 3, 4};

    qemu_irq **irqs, *mpic;

    DeviceState *dev;

    CPUPPCState *firstenv = NULL;

    MemoryRegion *ccsr_addr_space;

    SysBusDevice *s;

    PPCE500CCSRState *ccsr;



    /* Setup CPUs */

    if (args->cpu_model == NULL) {

        args->cpu_model = "e500v2_v30";

    }



    irqs = g_malloc0(smp_cpus * sizeof(qemu_irq *));

    irqs[0] = g_malloc0(smp_cpus * sizeof(qemu_irq) * OPENPIC_OUTPUT_NB);

    for (i = 0; i < smp_cpus; i++) {

        PowerPCCPU *cpu;

        CPUState *cs;

        qemu_irq *input;



        cpu = cpu_ppc_init(args->cpu_model);

        if (cpu == NULL) {

            fprintf(stderr, "Unable to initialize CPU!\n");

            exit(1);

        }

        env = &cpu->env;

        cs = CPU(cpu);



        if (!firstenv) {

            firstenv = env;

        }



        irqs[i] = irqs[0] + (i * OPENPIC_OUTPUT_NB);

        input = (qemu_irq *)env->irq_inputs;

        irqs[i][OPENPIC_OUTPUT_INT] = input[PPCE500_INPUT_INT];

        irqs[i][OPENPIC_OUTPUT_CINT] = input[PPCE500_INPUT_CINT];

        env->spr[SPR_BOOKE_PIR] = cs->cpu_index = i;

        env->mpic_iack = MPC8544_CCSRBAR_BASE +

                         MPC8544_MPIC_REGS_OFFSET + 0xa0;



        ppc_booke_timers_init(cpu, 400000000, PPC_TIMER_E500);



        /* Register reset handler */

        if (!i) {

            /* Primary CPU */

            struct boot_info *boot_info;

            boot_info = g_malloc0(sizeof(struct boot_info));

            qemu_register_reset(ppce500_cpu_reset, cpu);

            env->load_info = boot_info;

        } else {

            /* Secondary CPUs */

            qemu_register_reset(ppce500_cpu_reset_sec, cpu);

        }

    }



    env = firstenv;



    /* Fixup Memory size on a alignment boundary */

    ram_size &= ~(RAM_SIZES_ALIGN - 1);

    args->ram_size = ram_size;



    /* Register Memory */

    memory_region_init_ram(ram, NULL, "mpc8544ds.ram", ram_size);

    vmstate_register_ram_global(ram);

    memory_region_add_subregion(address_space_mem, 0, ram);



    dev = qdev_create(NULL, "e500-ccsr");

    object_property_add_child(qdev_get_machine(), "e500-ccsr",

                              OBJECT(dev), NULL);

    qdev_init_nofail(dev);

    ccsr = CCSR(dev);

    ccsr_addr_space = &ccsr->ccsr_space;

    memory_region_add_subregion(address_space_mem, MPC8544_CCSRBAR_BASE,

                                ccsr_addr_space);



    mpic = ppce500_init_mpic(params, ccsr_addr_space, irqs);



    /* Serial */

    if (serial_hds[0]) {

        serial_mm_init(ccsr_addr_space, MPC8544_SERIAL0_REGS_OFFSET,

                       0, mpic[42], 399193,

                       serial_hds[0], DEVICE_BIG_ENDIAN);

    }



    if (serial_hds[1]) {

        serial_mm_init(ccsr_addr_space, MPC8544_SERIAL1_REGS_OFFSET,

                       0, mpic[42], 399193,

                       serial_hds[1], DEVICE_BIG_ENDIAN);

    }



    /* General Utility device */

    dev = qdev_create(NULL, "mpc8544-guts");

    qdev_init_nofail(dev);

    s = SYS_BUS_DEVICE(dev);

    memory_region_add_subregion(ccsr_addr_space, MPC8544_UTIL_OFFSET,

                                sysbus_mmio_get_region(s, 0));



    /* PCI */

    dev = qdev_create(NULL, "e500-pcihost");

    qdev_prop_set_uint32(dev, "first_slot", params->pci_first_slot);

    qdev_init_nofail(dev);

    s = SYS_BUS_DEVICE(dev);

    sysbus_connect_irq(s, 0, mpic[pci_irq_nrs[0]]);

    sysbus_connect_irq(s, 1, mpic[pci_irq_nrs[1]]);

    sysbus_connect_irq(s, 2, mpic[pci_irq_nrs[2]]);

    sysbus_connect_irq(s, 3, mpic[pci_irq_nrs[3]]);

    memory_region_add_subregion(ccsr_addr_space, MPC8544_PCI_REGS_OFFSET,

                                sysbus_mmio_get_region(s, 0));



    pci_bus = (PCIBus *)qdev_get_child_bus(dev, "pci.0");

    if (!pci_bus)

        printf("couldn't create PCI controller!\n");



    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 1, MPC8544_PCI_IO);



    if (pci_bus) {

        /* Register network interfaces. */

        for (i = 0; i < nb_nics; i++) {

            pci_nic_init_nofail(&nd_table[i], pci_bus, "virtio", NULL);

        }

    }



    /* Register spinning region */

    sysbus_create_simple("e500-spin", MPC8544_SPIN_BASE, NULL);



    /* Load kernel. */

    if (args->kernel_filename) {

        kernel_size = load_uimage(args->kernel_filename, &entry,

                                  &loadaddr, NULL);

        if (kernel_size < 0) {

            kernel_size = load_elf(args->kernel_filename, NULL, NULL,

                                   &elf_entry, &elf_lowaddr, NULL, 1,

                                   ELF_MACHINE, 0);

            entry = elf_entry;

            loadaddr = elf_lowaddr;

        }

        /* XXX try again as binary */

        if (kernel_size < 0) {

            fprintf(stderr, "qemu: could not load kernel '%s'\n",

                    args->kernel_filename);

            exit(1);

        }



        cur_base = loadaddr + kernel_size;



        /* Reserve space for dtb */

        dt_base = (cur_base + DTC_LOAD_PAD) & ~DTC_PAD_MASK;

        cur_base += DTB_MAX_SIZE;

    }



    /* Load initrd. */

    if (args->initrd_filename) {

        initrd_base = (cur_base + INITRD_LOAD_PAD) & ~INITRD_PAD_MASK;

        initrd_size = load_image_targphys(args->initrd_filename, initrd_base,

                                          ram_size - initrd_base);



        if (initrd_size < 0) {

            fprintf(stderr, "qemu: could not load initial ram disk '%s'\n",

                    args->initrd_filename);

            exit(1);

        }



        cur_base = initrd_base + initrd_size;

    }



    /* If we're loading a kernel directly, we must load the device tree too. */

    if (args->kernel_filename) {

        struct boot_info *boot_info;

        int dt_size;



        dt_size = ppce500_load_device_tree(env, args, params, dt_base,

                                           initrd_base, initrd_size);

        if (dt_size < 0) {

            fprintf(stderr, "couldn't load device tree\n");

            exit(1);

        }

        assert(dt_size < DTB_MAX_SIZE);



        boot_info = env->load_info;

        boot_info->entry = entry;

        boot_info->dt_base = dt_base;

        boot_info->dt_size = dt_size;

    }



    if (kvm_enabled()) {

        kvmppc_init();

    }

}
