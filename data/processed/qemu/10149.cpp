static int xen_pt_byte_reg_write(XenPCIPassthroughState *s, XenPTReg *cfg_entry,

                                 uint8_t *val, uint8_t dev_value,

                                 uint8_t valid_mask)

{

    XenPTRegInfo *reg = cfg_entry->reg;

    uint8_t writable_mask = 0;

    uint8_t throughable_mask = get_throughable_mask(s, reg, valid_mask);



    /* modify emulate register */

    writable_mask = reg->emu_mask & ~reg->ro_mask & valid_mask;

    cfg_entry->data = XEN_PT_MERGE_VALUE(*val, cfg_entry->data, writable_mask);



    /* create value for writing to I/O device register */

    *val = XEN_PT_MERGE_VALUE(*val, dev_value, throughable_mask);



    return 0;

}
