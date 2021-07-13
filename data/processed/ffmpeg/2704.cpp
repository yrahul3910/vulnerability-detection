void *av_malloc(size_t size)

{

    void *ptr = NULL;

#if CONFIG_MEMALIGN_HACK

    long diff;

#endif



    /* let's disallow possible ambiguous cases */

    if (size > (max_alloc_size - 32))

        return NULL;



#if CONFIG_MEMALIGN_HACK

    ptr = malloc(size + ALIGN);

    if (!ptr)

        return ptr;

    diff              = ((~(long)ptr)&(ALIGN - 1)) + 1;

    ptr               = (char *)ptr + diff;

    ((char *)ptr)[-1] = diff;

#elif HAVE_POSIX_MEMALIGN

    if (size) //OS X on SDK 10.6 has a broken posix_memalign implementation

    if (posix_memalign(&ptr, ALIGN, size))

        ptr = NULL;

#elif HAVE_ALIGNED_MALLOC

    ptr = _aligned_malloc(size, ALIGN);

#elif HAVE_MEMALIGN

#ifndef __DJGPP__

    ptr = memalign(ALIGN, size);

#else

    ptr = memalign(size, ALIGN);

#endif

    /* Why 64?

     * Indeed, we should align it:

     *   on  4 for 386

     *   on 16 for 486

     *   on 32 for 586, PPro - K6-III

     *   on 64 for K7 (maybe for P3 too).

     * Because L1 and L2 caches are aligned on those values.

     * But I don't want to code such logic here!

     */

    /* Why 32?

     * For AVX ASM. SSE / NEON needs only 16.

     * Why not larger? Because I did not see a difference in benchmarks ...

     */

    /* benchmarks with P3

     * memalign(64) + 1          3071, 3051, 3032

     * memalign(64) + 2          3051, 3032, 3041

     * memalign(64) + 4          2911, 2896, 2915

     * memalign(64) + 8          2545, 2554, 2550

     * memalign(64) + 16         2543, 2572, 2563

     * memalign(64) + 32         2546, 2545, 2571

     * memalign(64) + 64         2570, 2533, 2558

     *

     * BTW, malloc seems to do 8-byte alignment by default here.

     */

#else

    ptr = malloc(size);

#endif

    if(!ptr && !size) {

        size = 1;

        ptr= av_malloc(1);

    }

#if CONFIG_MEMORY_POISONING

    if (ptr)

        memset(ptr, 0x2a, size);

#endif

    return ptr;

}
