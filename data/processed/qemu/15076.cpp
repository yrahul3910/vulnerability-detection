static bool fw_cfg_ctl_mem_valid(void *opaque, target_phys_addr_t addr,

                                 unsigned size, bool is_write)

{

    return is_write && size == 2;

}
