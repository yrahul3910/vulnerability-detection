static inline void RENAME(rgb15to32)(const uint8_t *src, uint8_t *dst, long src_size)

{

    const uint16_t *end;

#if COMPILE_TEMPLATE_MMX

    const uint16_t *mm_end;

#endif

    uint8_t *d = dst;

    const uint16_t *s = (const uint16_t *)src;

    end = s + src_size/2;

#if COMPILE_TEMPLATE_MMX

    __asm__ volatile(PREFETCH"    %0"::"m"(*s):"memory");

    __asm__ volatile("pxor    %%mm7,%%mm7    \n\t":::"memory");

    __asm__ volatile("pcmpeqd %%mm6,%%mm6    \n\t":::"memory");

    mm_end = end - 3;

    while (s < mm_end) {

        __asm__ volatile(

            PREFETCH"    32%1           \n\t"

            "movq          %1, %%mm0    \n\t"

            "movq          %1, %%mm1    \n\t"

            "movq          %1, %%mm2    \n\t"

            "pand          %2, %%mm0    \n\t"

            "pand          %3, %%mm1    \n\t"

            "pand          %4, %%mm2    \n\t"

            "psllq         $3, %%mm0    \n\t"

            "psrlq         $2, %%mm1    \n\t"

            "psrlq         $7, %%mm2    \n\t"

            PACK_RGB32

            :"=m"(*d)

            :"m"(*s),"m"(mask15b),"m"(mask15g),"m"(mask15r)

            :"memory");

        d += 16;

        s += 4;

    }

    __asm__ volatile(SFENCE:::"memory");

    __asm__ volatile(EMMS:::"memory");

#endif

    while (s < end) {

        register uint16_t bgr;

        bgr = *s++;

#if HAVE_BIGENDIAN

        *d++ = 255;

        *d++ = (bgr&0x7C00)>>7;

        *d++ = (bgr&0x3E0)>>2;

        *d++ = (bgr&0x1F)<<3;

#else

        *d++ = (bgr&0x1F)<<3;

        *d++ = (bgr&0x3E0)>>2;

        *d++ = (bgr&0x7C00)>>7;

        *d++ = 255;

#endif

    }

}
