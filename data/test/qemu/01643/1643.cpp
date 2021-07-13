static void fdt_add_gic_node(VirtBoardInfo *vbi)

{

    vbi->gic_phandle = qemu_fdt_alloc_phandle(vbi->fdt);

    qemu_fdt_setprop_cell(vbi->fdt, "/", "interrupt-parent", vbi->gic_phandle);



    qemu_fdt_add_subnode(vbi->fdt, "/intc");

    /* 'cortex-a15-gic' means 'GIC v2' */

    qemu_fdt_setprop_string(vbi->fdt, "/intc", "compatible",

                            "arm,cortex-a15-gic");

    qemu_fdt_setprop_cell(vbi->fdt, "/intc", "#interrupt-cells", 3);

    qemu_fdt_setprop(vbi->fdt, "/intc", "interrupt-controller", NULL, 0);

    qemu_fdt_setprop_sized_cells(vbi->fdt, "/intc", "reg",

                                     2, vbi->memmap[VIRT_GIC_DIST].base,

                                     2, vbi->memmap[VIRT_GIC_DIST].size,

                                     2, vbi->memmap[VIRT_GIC_CPU].base,

                                     2, vbi->memmap[VIRT_GIC_CPU].size);

    qemu_fdt_setprop_cell(vbi->fdt, "/intc", "#address-cells", 0x2);

    qemu_fdt_setprop_cell(vbi->fdt, "/intc", "#size-cells", 0x2);

    qemu_fdt_setprop(vbi->fdt, "/intc", "ranges", NULL, 0);

    qemu_fdt_setprop_cell(vbi->fdt, "/intc", "phandle", vbi->gic_phandle);

}
