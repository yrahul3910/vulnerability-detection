static void pc_init1(MachineState *machine,

                     const char *host_type, const char *pci_type)

{

    PCMachineState *pcms = PC_MACHINE(machine);

    PCMachineClass *pcmc = PC_MACHINE_GET_CLASS(pcms);

    MemoryRegion *system_memory = get_system_memory();

    MemoryRegion *system_io = get_system_io();

    int i;

    PCIBus *pci_bus;

    ISABus *isa_bus;

    PCII440FXState *i440fx_state;

    int piix3_devfn = -1;

    qemu_irq *i8259;

    qemu_irq smi_irq;

    GSIState *gsi_state;

    DriveInfo *hd[MAX_IDE_BUS * MAX_IDE_DEVS];

    BusState *idebus[MAX_IDE_BUS];

    ISADevice *rtc_state;

    MemoryRegion *ram_memory;

    MemoryRegion *pci_memory;

    MemoryRegion *rom_memory;

    ram_addr_t lowmem;



    /*

     * Calculate ram split, for memory below and above 4G.  It's a bit

     * complicated for backward compatibility reasons ...

     *

     *  - Traditional split is 3.5G (lowmem = 0xe0000000).  This is the

     *    default value for max_ram_below_4g now.

     *

     *  - Then, to gigabyte align the memory, we move the split to 3G

     *    (lowmem = 0xc0000000).  But only in case we have to split in

     *    the first place, i.e. ram_size is larger than (traditional)

     *    lowmem.  And for new machine types (gigabyte_align = true)

     *    only, for live migration compatibility reasons.

     *

     *  - Next the max-ram-below-4g option was added, which allowed to

     *    reduce lowmem to a smaller value, to allow a larger PCI I/O

     *    window below 4G.  qemu doesn't enforce gigabyte alignment here,

     *    but prints a warning.

     *

     *  - Finally max-ram-below-4g got updated to also allow raising lowmem,

     *    so legacy non-PAE guests can get as much memory as possible in

     *    the 32bit address space below 4G.

     *

     *  - Note that Xen has its own ram setp code in xen_ram_init(),

     *    called via xen_hvm_init().

     *

     * Examples:

     *    qemu -M pc-1.7 -m 4G    (old default)    -> 3584M low,  512M high

     *    qemu -M pc -m 4G        (new default)    -> 3072M low, 1024M high

     *    qemu -M pc,max-ram-below-4g=2G -m 4G     -> 2048M low, 2048M high

     *    qemu -M pc,max-ram-below-4g=4G -m 3968M  -> 3968M low (=4G-128M)

     */

    if (xen_enabled()) {

        xen_hvm_init(pcms, &ram_memory);

    } else {

        if (!pcms->max_ram_below_4g) {

            pcms->max_ram_below_4g = 0xe0000000; /* default: 3.5G */

        }

        lowmem = pcms->max_ram_below_4g;

        if (machine->ram_size >= pcms->max_ram_below_4g) {

            if (pcmc->gigabyte_align) {

                if (lowmem > 0xc0000000) {

                    lowmem = 0xc0000000;

                }

                if (lowmem & ((1ULL << 30) - 1)) {

                    error_report("Warning: Large machine and max_ram_below_4g "

                                 "(%" PRIu64 ") not a multiple of 1G; "

                                 "possible bad performance.",

                                 pcms->max_ram_below_4g);

                }

            }

        }



        if (machine->ram_size >= lowmem) {

            pcms->above_4g_mem_size = machine->ram_size - lowmem;

            pcms->below_4g_mem_size = lowmem;

        } else {

            pcms->above_4g_mem_size = 0;

            pcms->below_4g_mem_size = machine->ram_size;

        }

    }



    pc_cpus_init(pcms);



    if (kvm_enabled() && pcmc->kvmclock_enabled) {

        kvmclock_create();

    }



    if (pcmc->pci_enabled) {

        pci_memory = g_new(MemoryRegion, 1);

        memory_region_init(pci_memory, NULL, "pci", UINT64_MAX);

        rom_memory = pci_memory;

    } else {

        pci_memory = NULL;

        rom_memory = system_memory;

    }



    pc_guest_info_init(pcms);



    if (pcmc->smbios_defaults) {

        MachineClass *mc = MACHINE_GET_CLASS(machine);

        /* These values are guest ABI, do not change */

        smbios_set_defaults("QEMU", "Standard PC (i440FX + PIIX, 1996)",

                            mc->name, pcmc->smbios_legacy_mode,

                            pcmc->smbios_uuid_encoded,

                            SMBIOS_ENTRY_POINT_21);

    }



    /* allocate ram and load rom/bios */

    if (!xen_enabled()) {

        pc_memory_init(pcms, system_memory,

                       rom_memory, &ram_memory);

    } else if (machine->kernel_filename != NULL) {

        /* For xen HVM direct kernel boot, load linux here */

        xen_load_linux(pcms);

    }



    gsi_state = g_malloc0(sizeof(*gsi_state));

    if (kvm_ioapic_in_kernel()) {

        kvm_pc_setup_irq_routing(pcmc->pci_enabled);

        pcms->gsi = qemu_allocate_irqs(kvm_pc_gsi_handler, gsi_state,

                                       GSI_NUM_PINS);

    } else {

        pcms->gsi = qemu_allocate_irqs(gsi_handler, gsi_state, GSI_NUM_PINS);

    }



    if (pcmc->pci_enabled) {

        pci_bus = i440fx_init(host_type,

                              pci_type,

                              &i440fx_state, &piix3_devfn, &isa_bus, pcms->gsi,

                              system_memory, system_io, machine->ram_size,

                              pcms->below_4g_mem_size,

                              pcms->above_4g_mem_size,

                              pci_memory, ram_memory);

        pcms->bus = pci_bus;

    } else {

        pci_bus = NULL;

        i440fx_state = NULL;

        isa_bus = isa_bus_new(NULL, get_system_memory(), system_io,

                              &error_abort);

        no_hpet = 1;

    }

    isa_bus_irqs(isa_bus, pcms->gsi);



    if (kvm_pic_in_kernel()) {

        i8259 = kvm_i8259_init(isa_bus);

    } else if (xen_enabled()) {

        i8259 = xen_interrupt_controller_init();

    } else {

        i8259 = i8259_init(isa_bus, pc_allocate_cpu_irq());

    }



    for (i = 0; i < ISA_NUM_IRQS; i++) {

        gsi_state->i8259_irq[i] = i8259[i];

    }

    g_free(i8259);

    if (pcmc->pci_enabled) {

        ioapic_init_gsi(gsi_state, "i440fx");

    }



    pc_register_ferr_irq(pcms->gsi[13]);



    pc_vga_init(isa_bus, pcmc->pci_enabled ? pci_bus : NULL);



    assert(pcms->vmport != ON_OFF_AUTO__MAX);

    if (pcms->vmport == ON_OFF_AUTO_AUTO) {

        pcms->vmport = xen_enabled() ? ON_OFF_AUTO_OFF : ON_OFF_AUTO_ON;

    }



    /* init basic PC hardware */

    pc_basic_device_init(isa_bus, pcms->gsi, &rtc_state, true,

                         (pcms->vmport != ON_OFF_AUTO_ON), pcms->pit, 0x4);



    pc_nic_init(isa_bus, pci_bus);



    ide_drive_get(hd, ARRAY_SIZE(hd));

    if (pcmc->pci_enabled) {

        PCIDevice *dev;

        if (xen_enabled()) {

            dev = pci_piix3_xen_ide_init(pci_bus, hd, piix3_devfn + 1);

        } else {

            dev = pci_piix3_ide_init(pci_bus, hd, piix3_devfn + 1);

        }

        idebus[0] = qdev_get_child_bus(&dev->qdev, "ide.0");

        idebus[1] = qdev_get_child_bus(&dev->qdev, "ide.1");

    } else {

        for(i = 0; i < MAX_IDE_BUS; i++) {

            ISADevice *dev;

            char busname[] = "ide.0";

            dev = isa_ide_init(isa_bus, ide_iobase[i], ide_iobase2[i],

                               ide_irq[i],

                               hd[MAX_IDE_DEVS * i], hd[MAX_IDE_DEVS * i + 1]);

            /*

             * The ide bus name is ide.0 for the first bus and ide.1 for the

             * second one.

             */

            busname[4] = '0' + i;

            idebus[i] = qdev_get_child_bus(DEVICE(dev), busname);

        }

    }



    pc_cmos_init(pcms, idebus[0], idebus[1], rtc_state);



    if (pcmc->pci_enabled && machine_usb(machine)) {

        pci_create_simple(pci_bus, piix3_devfn + 2, "piix3-usb-uhci");

    }



    if (pcmc->pci_enabled && acpi_enabled) {

        DeviceState *piix4_pm;

        I2CBus *smbus;



        smi_irq = qemu_allocate_irq(pc_acpi_smi_interrupt, first_cpu, 0);

        /* TODO: Populate SPD eeprom data.  */

        smbus = piix4_pm_init(pci_bus, piix3_devfn + 3, 0xb100,

                              pcms->gsi[9], smi_irq,

                              pc_machine_is_smm_enabled(pcms),

                              &piix4_pm);

        smbus_eeprom_init(smbus, 8, NULL, 0);



        object_property_add_link(OBJECT(machine), PC_MACHINE_ACPI_DEVICE_PROP,

                                 TYPE_HOTPLUG_HANDLER,

                                 (Object **)&pcms->acpi_dev,

                                 object_property_allow_set_link,

                                 OBJ_PROP_LINK_UNREF_ON_RELEASE, &error_abort);

        object_property_set_link(OBJECT(machine), OBJECT(piix4_pm),

                                 PC_MACHINE_ACPI_DEVICE_PROP, &error_abort);

    }



    if (pcmc->pci_enabled) {

        pc_pci_device_init(pci_bus);

    }



    if (pcms->acpi_nvdimm_state.is_enabled) {

        nvdimm_init_acpi_state(&pcms->acpi_nvdimm_state, system_io,

                               pcms->fw_cfg, OBJECT(pcms));

    }

}
