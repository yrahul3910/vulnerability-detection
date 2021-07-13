static inline void stw_phys_internal(hwaddr addr, uint32_t val,

                                     enum device_endian endian)

{

    uint8_t *ptr;

    MemoryRegionSection *section;



    section = phys_page_find(address_space_memory.dispatch, addr >> TARGET_PAGE_BITS);



    if (!memory_region_is_ram(section->mr) || section->readonly) {

        addr = memory_region_section_addr(section, addr);

        if (memory_region_is_ram(section->mr)) {

            section = &phys_sections[phys_section_rom];

        }

#if defined(TARGET_WORDS_BIGENDIAN)

        if (endian == DEVICE_LITTLE_ENDIAN) {

            val = bswap16(val);

        }

#else

        if (endian == DEVICE_BIG_ENDIAN) {

            val = bswap16(val);

        }

#endif

        io_mem_write(section->mr, addr, val, 2);

    } else {

        unsigned long addr1;

        addr1 = (memory_region_get_ram_addr(section->mr) & TARGET_PAGE_MASK)

            + memory_region_section_addr(section, addr);

        /* RAM case */

        ptr = qemu_get_ram_ptr(addr1);

        switch (endian) {

        case DEVICE_LITTLE_ENDIAN:

            stw_le_p(ptr, val);

            break;

        case DEVICE_BIG_ENDIAN:

            stw_be_p(ptr, val);

            break;

        default:

            stw_p(ptr, val);

            break;

        }

        invalidate_and_set_dirty(addr1, 2);

    }

}
