void page_set_flags(target_ulong start, target_ulong end, int flags)

{

    target_ulong addr, len;



    /* This function should never be called with addresses outside the

       guest address space.  If this assert fires, it probably indicates

       a missing call to h2g_valid.  */

#if TARGET_ABI_BITS > L1_MAP_ADDR_SPACE_BITS

    assert(end < ((target_ulong)1 << L1_MAP_ADDR_SPACE_BITS));

#endif

    assert(start < end);

    assert_memory_lock();



    start = start & TARGET_PAGE_MASK;

    end = TARGET_PAGE_ALIGN(end);



    if (flags & PAGE_WRITE) {

        flags |= PAGE_WRITE_ORG;

    }



    for (addr = start, len = end - start;

         len != 0;

         len -= TARGET_PAGE_SIZE, addr += TARGET_PAGE_SIZE) {

        PageDesc *p = page_find_alloc(addr >> TARGET_PAGE_BITS, 1);



        /* If the write protection bit is set, then we invalidate

           the code inside.  */

        if (!(p->flags & PAGE_WRITE) &&

            (flags & PAGE_WRITE) &&

            p->first_tb) {

            tb_invalidate_phys_page(addr, 0);

        }

        p->flags = flags;

    }

}
