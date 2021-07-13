void av_fast_malloc(void *ptr, unsigned int *size, size_t min_size)

{

    void **p = ptr;

    if (min_size < *size)

        return;

    min_size= FFMAX(17*min_size/16 + 32, min_size);

    av_free(*p);

    *p = av_malloc(min_size);

    if (!*p) min_size = 0;

    *size= min_size;

}
