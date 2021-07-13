static inline void idct4col_add(uint8_t *dest, int line_size, const DCTELEM *col)

{

    int c0, c1, c2, c3, a0, a1, a2, a3;

    const uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;



    a0 = col[8*0];

    a1 = col[8*1];

    a2 = col[8*2];

    a3 = col[8*3];

    c0 = (a0 + a2)*C3 + (1 << (C_SHIFT - 1));

    c2 = (a0 - a2)*C3 + (1 << (C_SHIFT - 1));

    c1 = a1 * C1 + a3 * C2;

    c3 = a1 * C2 - a3 * C1;

    dest[0] = cm[dest[0] + ((c0 + c1) >> C_SHIFT)];

    dest += line_size;

    dest[0] = cm[dest[0] + ((c2 + c3) >> C_SHIFT)];

    dest += line_size;

    dest[0] = cm[dest[0] + ((c2 - c3) >> C_SHIFT)];

    dest += line_size;

    dest[0] = cm[dest[0] + ((c0 - c1) >> C_SHIFT)];

}
