static void i440fx_pcihost_get_pci_hole64_end(Object *obj, Visitor *v,

                                              const char *name, void *opaque,

                                              Error **errp)

{

    PCIHostState *h = PCI_HOST_BRIDGE(obj);

    Range w64;



    pci_bus_get_w64_range(h->bus, &w64);



    visit_type_uint64(v, name, &w64.end, errp);

}
