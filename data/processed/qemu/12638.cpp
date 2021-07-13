static void q35_host_get_pci_hole_start(Object *obj, Visitor *v,

                                        const char *name, void *opaque,

                                        Error **errp)

{

    Q35PCIHost *s = Q35_HOST_DEVICE(obj);

    uint32_t value = s->mch.pci_hole.begin;



    visit_type_uint32(v, name, &value, errp);

}
