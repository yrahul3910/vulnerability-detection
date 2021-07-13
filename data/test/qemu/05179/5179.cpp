static int kvm_get_dirty_pages_log_range(MemoryRegionSection *section,

                                         unsigned long *bitmap)

{

    unsigned int i, j;

    unsigned long page_number, c;

    hwaddr addr, addr1;

    unsigned int len = ((section->size / TARGET_PAGE_SIZE) + HOST_LONG_BITS - 1) / HOST_LONG_BITS;

    unsigned long hpratio = getpagesize() / TARGET_PAGE_SIZE;



    /*

     * bitmap-traveling is faster than memory-traveling (for addr...)

     * especially when most of the memory is not dirty.

     */

    for (i = 0; i < len; i++) {

        if (bitmap[i] != 0) {

            c = leul_to_cpu(bitmap[i]);

            do {

                j = ffsl(c) - 1;

                c &= ~(1ul << j);

                page_number = (i * HOST_LONG_BITS + j) * hpratio;

                addr1 = page_number * TARGET_PAGE_SIZE;

                addr = section->offset_within_region + addr1;

                memory_region_set_dirty(section->mr, addr,

                                        TARGET_PAGE_SIZE * hpratio);

            } while (c != 0);

        }

    }

    return 0;

}
