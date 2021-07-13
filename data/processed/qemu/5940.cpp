int spapr_populate_pci_dt(sPAPRPHBState *phb,

                          uint32_t xics_phandle,

                          void *fdt)

{

    int bus_off, i, j, ret;

    char nodename[FDT_NAME_MAX];

    uint32_t bus_range[] = { cpu_to_be32(0), cpu_to_be32(0xff) };

    const uint64_t mmiosize = memory_region_size(&phb->memwindow);

    const uint64_t w32max = (1ULL << 32) - SPAPR_PCI_MEM_WIN_BUS_OFFSET;

    const uint64_t w32size = MIN(w32max, mmiosize);

    const uint64_t w64size = (mmiosize > w32size) ? (mmiosize - w32size) : 0;

    struct {

        uint32_t hi;

        uint64_t child;

        uint64_t parent;

        uint64_t size;

    } QEMU_PACKED ranges[] = {

        {

            cpu_to_be32(b_ss(1)), cpu_to_be64(0),

            cpu_to_be64(phb->io_win_addr),

            cpu_to_be64(memory_region_size(&phb->iospace)),

        },

        {

            cpu_to_be32(b_ss(2)), cpu_to_be64(SPAPR_PCI_MEM_WIN_BUS_OFFSET),

            cpu_to_be64(phb->mem_win_addr),

            cpu_to_be64(w32size),

        },

        {

            cpu_to_be32(b_ss(3)), cpu_to_be64(1ULL << 32),

            cpu_to_be64(phb->mem_win_addr + w32size),

            cpu_to_be64(w64size)

        },

    };

    const unsigned sizeof_ranges = (w64size ? 3 : 2) * sizeof(ranges[0]);

    uint64_t bus_reg[] = { cpu_to_be64(phb->buid), 0 };

    uint32_t interrupt_map_mask[] = {

        cpu_to_be32(b_ddddd(-1)|b_fff(0)), 0x0, 0x0, cpu_to_be32(-1)};

    uint32_t interrupt_map[PCI_SLOT_MAX * PCI_NUM_PINS][7];

    uint32_t ddw_applicable[] = {

        cpu_to_be32(RTAS_IBM_QUERY_PE_DMA_WINDOW),

        cpu_to_be32(RTAS_IBM_CREATE_PE_DMA_WINDOW),

        cpu_to_be32(RTAS_IBM_REMOVE_PE_DMA_WINDOW)

    };

    uint32_t ddw_extensions[] = {

        cpu_to_be32(1),

        cpu_to_be32(RTAS_IBM_RESET_PE_DMA_WINDOW)

    };

    uint32_t associativity[] = {cpu_to_be32(0x4),

                                cpu_to_be32(0x0),

                                cpu_to_be32(0x0),

                                cpu_to_be32(0x0),

                                cpu_to_be32(phb->numa_node)};

    sPAPRTCETable *tcet;

    PCIBus *bus = PCI_HOST_BRIDGE(phb)->bus;

    sPAPRFDT s_fdt;



    /* Start populating the FDT */

    snprintf(nodename, FDT_NAME_MAX, "pci@%" PRIx64, phb->buid);

    bus_off = fdt_add_subnode(fdt, 0, nodename);

    if (bus_off < 0) {

        return bus_off;

    }



    /* Write PHB properties */

    _FDT(fdt_setprop_string(fdt, bus_off, "device_type", "pci"));

    _FDT(fdt_setprop_string(fdt, bus_off, "compatible", "IBM,Logical_PHB"));

    _FDT(fdt_setprop_cell(fdt, bus_off, "#address-cells", 0x3));

    _FDT(fdt_setprop_cell(fdt, bus_off, "#size-cells", 0x2));

    _FDT(fdt_setprop_cell(fdt, bus_off, "#interrupt-cells", 0x1));

    _FDT(fdt_setprop(fdt, bus_off, "used-by-rtas", NULL, 0));

    _FDT(fdt_setprop(fdt, bus_off, "bus-range", &bus_range, sizeof(bus_range)));

    _FDT(fdt_setprop(fdt, bus_off, "ranges", &ranges, sizeof_ranges));

    _FDT(fdt_setprop(fdt, bus_off, "reg", &bus_reg, sizeof(bus_reg)));

    _FDT(fdt_setprop_cell(fdt, bus_off, "ibm,pci-config-space-type", 0x1));

    _FDT(fdt_setprop_cell(fdt, bus_off, "ibm,pe-total-#msi", XICS_IRQS_SPAPR));



    /* Dynamic DMA window */

    if (phb->ddw_enabled) {

        _FDT(fdt_setprop(fdt, bus_off, "ibm,ddw-applicable", &ddw_applicable,

                         sizeof(ddw_applicable)));

        _FDT(fdt_setprop(fdt, bus_off, "ibm,ddw-extensions",

                         &ddw_extensions, sizeof(ddw_extensions)));

    }



    /* Advertise NUMA via ibm,associativity */

    if (nb_numa_nodes > 1) {

        _FDT(fdt_setprop(fdt, bus_off, "ibm,associativity", associativity,

                         sizeof(associativity)));

    }



    /* Build the interrupt-map, this must matches what is done

     * in pci_spapr_map_irq

     */

    _FDT(fdt_setprop(fdt, bus_off, "interrupt-map-mask",

                     &interrupt_map_mask, sizeof(interrupt_map_mask)));

    for (i = 0; i < PCI_SLOT_MAX; i++) {

        for (j = 0; j < PCI_NUM_PINS; j++) {

            uint32_t *irqmap = interrupt_map[i*PCI_NUM_PINS + j];

            int lsi_num = pci_spapr_swizzle(i, j);



            irqmap[0] = cpu_to_be32(b_ddddd(i)|b_fff(0));

            irqmap[1] = 0;

            irqmap[2] = 0;

            irqmap[3] = cpu_to_be32(j+1);

            irqmap[4] = cpu_to_be32(xics_phandle);

            irqmap[5] = cpu_to_be32(phb->lsi_table[lsi_num].irq);

            irqmap[6] = cpu_to_be32(0x8);

        }

    }

    /* Write interrupt map */

    _FDT(fdt_setprop(fdt, bus_off, "interrupt-map", &interrupt_map,

                     sizeof(interrupt_map)));



    tcet = spapr_tce_find_by_liobn(phb->dma_liobn[0]);

    if (!tcet) {

        return -1;

    }

    spapr_dma_dt(fdt, bus_off, "ibm,dma-window",

                 tcet->liobn, tcet->bus_offset,

                 tcet->nb_table << tcet->page_shift);



    /* Walk the bridges and program the bus numbers*/

    spapr_phb_pci_enumerate(phb);

    _FDT(fdt_setprop_cell(fdt, bus_off, "qemu,phb-enumerated", 0x1));



    /* Populate tree nodes with PCI devices attached */

    s_fdt.fdt = fdt;

    s_fdt.node_off = bus_off;

    s_fdt.sphb = phb;

    pci_for_each_device(bus, pci_bus_num(bus),

                        spapr_populate_pci_devices_dt,

                        &s_fdt);



    ret = spapr_drc_populate_dt(fdt, bus_off, OBJECT(phb),

                                SPAPR_DR_CONNECTOR_TYPE_PCI);

    if (ret) {

        return ret;

    }



    return 0;

}
