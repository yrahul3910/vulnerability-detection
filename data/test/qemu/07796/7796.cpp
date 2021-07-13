void qemu_ram_remap(ram_addr_t addr, ram_addr_t length)

{

    RAMBlock *block;

    ram_addr_t offset;

    int flags;

    void *area, *vaddr;



    QTAILQ_FOREACH(block, &ram_list.blocks, next) {

        offset = addr - block->offset;

        if (offset < block->length) {

            vaddr = block->host + offset;

            if (block->flags & RAM_PREALLOC_MASK) {

                ;

            } else if (xen_enabled()) {

                abort();

            } else {

                flags = MAP_FIXED;

                munmap(vaddr, length);

                if (mem_path) {

#if defined(__linux__) && !defined(TARGET_S390X)

                    if (block->fd) {

#ifdef MAP_POPULATE

                        flags |= mem_prealloc ? MAP_POPULATE | MAP_SHARED :

                            MAP_PRIVATE;

#else

                        flags |= MAP_PRIVATE;

#endif

                        area = mmap(vaddr, length, PROT_READ | PROT_WRITE,

                                    flags, block->fd, offset);

                    } else {

                        flags |= MAP_PRIVATE | MAP_ANONYMOUS;

                        area = mmap(vaddr, length, PROT_READ | PROT_WRITE,

                                    flags, -1, 0);

                    }

#else

                    abort();

#endif

                } else {

#if defined(TARGET_S390X) && defined(CONFIG_KVM)

                    flags |= MAP_SHARED | MAP_ANONYMOUS;

                    area = mmap(vaddr, length, PROT_EXEC|PROT_READ|PROT_WRITE,

                                flags, -1, 0);

#else

                    flags |= MAP_PRIVATE | MAP_ANONYMOUS;

                    area = mmap(vaddr, length, PROT_READ | PROT_WRITE,

                                flags, -1, 0);

#endif

                }

                if (area != vaddr) {

                    fprintf(stderr, "Could not remap addr: "

                            RAM_ADDR_FMT "@" RAM_ADDR_FMT "\n",

                            length, addr);

                    exit(1);

                }

                memory_try_enable_merging(vaddr, length);

                qemu_ram_setup_dump(vaddr, length);

            }

            return;

        }

    }

}
