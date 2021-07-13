static inline void RENAME(bgr24ToUV)(uint8_t *dstU, uint8_t *dstV, uint8_t *src1, uint8_t *src2, long width)

{

#ifdef HAVE_MMX

	asm volatile(

		"mov %3, %%"REG_a"		\n\t"

		"movq "MANGLE(w1111)", %%mm5		\n\t"

		"movq "MANGLE(bgr2UCoeff)", %%mm6		\n\t"

		"pxor %%mm7, %%mm7		\n\t"

		"lea (%%"REG_a", %%"REG_a", 2), %%"REG_d"	\n\t"

		"add %%"REG_d", %%"REG_d"	\n\t"

		ASMALIGN(4)

		"1:				\n\t"

		PREFETCH" 64(%0, %%"REG_d")	\n\t"

#if defined (HAVE_MMX2) || defined (HAVE_3DNOW)

		"movq (%0, %%"REG_d"), %%mm0	\n\t"

		"movq 6(%0, %%"REG_d"), %%mm2	\n\t"

		"movq %%mm0, %%mm1		\n\t"

		"movq %%mm2, %%mm3		\n\t"

		"psrlq $24, %%mm0		\n\t"

		"psrlq $24, %%mm2		\n\t"

		PAVGB(%%mm1, %%mm0)

		PAVGB(%%mm3, %%mm2)

		"punpcklbw %%mm7, %%mm0		\n\t"

		"punpcklbw %%mm7, %%mm2		\n\t"

#else

		"movd (%0, %%"REG_d"), %%mm0	\n\t"

		"movd 3(%0, %%"REG_d"), %%mm2	\n\t"

		"punpcklbw %%mm7, %%mm0		\n\t"

		"punpcklbw %%mm7, %%mm2		\n\t"

		"paddw %%mm2, %%mm0		\n\t"

		"movd 6(%0, %%"REG_d"), %%mm4	\n\t"

		"movd 9(%0, %%"REG_d"), %%mm2	\n\t"

		"punpcklbw %%mm7, %%mm4		\n\t"

		"punpcklbw %%mm7, %%mm2		\n\t"

		"paddw %%mm4, %%mm2		\n\t"

		"psrlw $1, %%mm0		\n\t"

		"psrlw $1, %%mm2		\n\t"

#endif

		"movq "MANGLE(bgr2VCoeff)", %%mm1		\n\t"

		"movq "MANGLE(bgr2VCoeff)", %%mm3		\n\t"



		"pmaddwd %%mm0, %%mm1		\n\t"

		"pmaddwd %%mm2, %%mm3		\n\t"

		"pmaddwd %%mm6, %%mm0		\n\t"

		"pmaddwd %%mm6, %%mm2		\n\t"

#ifndef FAST_BGR2YV12

		"psrad $8, %%mm0		\n\t"

		"psrad $8, %%mm1		\n\t"

		"psrad $8, %%mm2		\n\t"

		"psrad $8, %%mm3		\n\t"

#endif

		"packssdw %%mm2, %%mm0		\n\t"

		"packssdw %%mm3, %%mm1		\n\t"

		"pmaddwd %%mm5, %%mm0		\n\t"

		"pmaddwd %%mm5, %%mm1		\n\t"

		"packssdw %%mm1, %%mm0		\n\t" // V1 V0 U1 U0

		"psraw $7, %%mm0		\n\t"



#if defined (HAVE_MMX2) || defined (HAVE_3DNOW)

		"movq 12(%0, %%"REG_d"), %%mm4	\n\t"

		"movq 18(%0, %%"REG_d"), %%mm2	\n\t"

		"movq %%mm4, %%mm1		\n\t"

		"movq %%mm2, %%mm3		\n\t"

		"psrlq $24, %%mm4		\n\t"

		"psrlq $24, %%mm2		\n\t"

		PAVGB(%%mm1, %%mm4)

		PAVGB(%%mm3, %%mm2)

		"punpcklbw %%mm7, %%mm4		\n\t"

		"punpcklbw %%mm7, %%mm2		\n\t"

#else

		"movd 12(%0, %%"REG_d"), %%mm4	\n\t"

		"movd 15(%0, %%"REG_d"), %%mm2	\n\t"

		"punpcklbw %%mm7, %%mm4		\n\t"

		"punpcklbw %%mm7, %%mm2		\n\t"

		"paddw %%mm2, %%mm4		\n\t"

		"movd 18(%0, %%"REG_d"), %%mm5	\n\t"

		"movd 21(%0, %%"REG_d"), %%mm2	\n\t"

		"punpcklbw %%mm7, %%mm5		\n\t"

		"punpcklbw %%mm7, %%mm2		\n\t"

		"paddw %%mm5, %%mm2		\n\t"

		"movq "MANGLE(w1111)", %%mm5		\n\t"

		"psrlw $2, %%mm4		\n\t"

		"psrlw $2, %%mm2		\n\t"

#endif

		"movq "MANGLE(bgr2VCoeff)", %%mm1		\n\t"

		"movq "MANGLE(bgr2VCoeff)", %%mm3		\n\t"



		"pmaddwd %%mm4, %%mm1		\n\t"

		"pmaddwd %%mm2, %%mm3		\n\t"

		"pmaddwd %%mm6, %%mm4		\n\t"

		"pmaddwd %%mm6, %%mm2		\n\t"

#ifndef FAST_BGR2YV12

		"psrad $8, %%mm4		\n\t"

		"psrad $8, %%mm1		\n\t"

		"psrad $8, %%mm2		\n\t"

		"psrad $8, %%mm3		\n\t"

#endif

		"packssdw %%mm2, %%mm4		\n\t"

		"packssdw %%mm3, %%mm1		\n\t"

		"pmaddwd %%mm5, %%mm4		\n\t"

		"pmaddwd %%mm5, %%mm1		\n\t"

		"add $24, %%"REG_d"		\n\t"

		"packssdw %%mm1, %%mm4		\n\t" // V3 V2 U3 U2

		"psraw $7, %%mm4		\n\t"



		"movq %%mm0, %%mm1		\n\t"

		"punpckldq %%mm4, %%mm0		\n\t"

		"punpckhdq %%mm4, %%mm1		\n\t"

		"packsswb %%mm1, %%mm0		\n\t"

		"paddb "MANGLE(bgr2UVOffset)", %%mm0	\n\t"



		"movd %%mm0, (%1, %%"REG_a")	\n\t"

		"punpckhdq %%mm0, %%mm0		\n\t"

		"movd %%mm0, (%2, %%"REG_a")	\n\t"

		"add $4, %%"REG_a"		\n\t"

		" js 1b				\n\t"

		: : "r" (src1+width*6), "r" (dstU+width), "r" (dstV+width), "g" (-width)

		: "%"REG_a, "%"REG_d

	);

#else

	int i;

	for(i=0; i<width; i++)

	{

		int b= src1[6*i + 0] + src1[6*i + 3];

		int g= src1[6*i + 1] + src1[6*i + 4];

		int r= src1[6*i + 2] + src1[6*i + 5];



		dstU[i]= ((RU*r + GU*g + BU*b)>>(RGB2YUV_SHIFT+1)) + 128;

		dstV[i]= ((RV*r + GV*g + BV*b)>>(RGB2YUV_SHIFT+1)) + 128;

	}

#endif

        assert(src1 == src2);

}
