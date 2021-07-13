static uint64_t memory_region_dispatch_read1(MemoryRegion *mr,

                                             hwaddr addr,

                                             unsigned size)

{

    uint64_t data = 0;



    if (!memory_region_access_valid(mr, addr, size, false)) {

        return -1U; /* FIXME: better signalling */

    }



    if (!mr->ops->read) {

        return mr->ops->old_mmio.read[ctz32(size)](mr->opaque, addr);

    }



    /* FIXME: support unaligned access */

    access_with_adjusted_size(addr, &data, size,

                              mr->ops->impl.min_access_size,

                              mr->ops->impl.max_access_size,

                              memory_region_read_accessor, mr);



    return data;

}
