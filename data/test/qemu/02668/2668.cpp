void *qemu_ram_mmap(int fd, size_t size, size_t align, bool shared)

{

    /*

     * Note: this always allocates at least one extra page of virtual address

     * space, even if size is already aligned.

     */

    size_t total = size + align;

#if defined(__powerpc64__) && defined(__linux__)

    /* On ppc64 mappings in the same segment (aka slice) must share the same

     * page size. Since we will be re-allocating part of this segment

     * from the supplied fd, we should make sure to use the same page size,

     * unless we are using the system page size, in which case anonymous memory

     * is OK. Use align as a hint for the page size.

     * In this case, set MAP_NORESERVE to avoid allocating backing store memory.

     */

    int anonfd = fd == -1 || qemu_fd_getpagesize(fd) == getpagesize() ? -1 : fd;

    int flags = anonfd == -1 ? MAP_ANONYMOUS : MAP_NORESERVE;

    void *ptr = mmap(0, total, PROT_NONE, flags | MAP_PRIVATE, anonfd, 0);

#else

    void *ptr = mmap(0, total, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

#endif

    size_t offset = QEMU_ALIGN_UP((uintptr_t)ptr, align) - (uintptr_t)ptr;

    void *ptr1;



    if (ptr == MAP_FAILED) {

        return MAP_FAILED;

    }



    /* Make sure align is a power of 2 */

    assert(!(align & (align - 1)));

    /* Always align to host page size */

    assert(align >= getpagesize());



    ptr1 = mmap(ptr + offset, size, PROT_READ | PROT_WRITE,

                MAP_FIXED |

                (fd == -1 ? MAP_ANONYMOUS : 0) |

                (shared ? MAP_SHARED : MAP_PRIVATE),

                fd, 0);

    if (ptr1 == MAP_FAILED) {

        munmap(ptr, total);

        return MAP_FAILED;

    }



    ptr += offset;

    total -= offset;



    if (offset > 0) {

        munmap(ptr - offset, offset);

    }



    /*

     * Leave a single PROT_NONE page allocated after the RAM block, to serve as

     * a guard page guarding against potential buffer overflows.

     */

    if (total > size + getpagesize()) {

        munmap(ptr + size + getpagesize(), total - size - getpagesize());

    }



    return ptr;

}
