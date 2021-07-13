void cpu_register_physical_memory_log(MemoryRegionSection *section,

                                      bool readonly)

{

    MemoryRegionSection now = *section, remain = *section;



    if ((now.offset_within_address_space & ~TARGET_PAGE_MASK)

        || (now.size < TARGET_PAGE_SIZE)) {

        now.size = MIN(TARGET_PAGE_ALIGN(now.offset_within_address_space)

                       - now.offset_within_address_space,

                       now.size);

        register_subpage(&now);

        remain.size -= now.size;

        remain.offset_within_address_space += now.size;

        remain.offset_within_region += now.size;

    }

    while (remain.size >= TARGET_PAGE_SIZE) {

        now = remain;

        if (remain.offset_within_region & ~TARGET_PAGE_MASK) {

            now.size = TARGET_PAGE_SIZE;

            register_subpage(&now);

        } else {

            now.size &= TARGET_PAGE_MASK;

            register_multipage(&now);

        }

        remain.size -= now.size;

        remain.offset_within_address_space += now.size;

        remain.offset_within_region += now.size;

    }

    now = remain;

    if (now.size) {

        register_subpage(&now);

    }

}
