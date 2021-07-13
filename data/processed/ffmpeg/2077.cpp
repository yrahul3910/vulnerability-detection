static void sha1_transform(uint32_t state[5], const uint8_t buffer[64])

{

    uint32_t block[80];

    unsigned int i, a, b, c, d, e;



    a = state[0];

    b = state[1];

    c = state[2];

    d = state[3];

    e = state[4];

#if CONFIG_SMALL

    for (i = 0; i < 80; i++) {

        int t;

        if (i < 16)

            t = AV_RB32(buffer + 4 * i);

        else

            t = rol(block[i-3] ^ block[i-8] ^ block[i-14] ^ block[i-16], 1);

        block[i] = t;

        t += e + rol(a, 5);

        if (i < 40) {

            if (i < 20)

                t += ((b&(c^d))^d)     + 0x5A827999;

            else

                t += ( b^c     ^d)     + 0x6ED9EBA1;

        } else {

            if (i < 60)

                t += (((b|c)&d)|(b&c)) + 0x8F1BBCDC;

            else

                t += ( b^c     ^d)     + 0xCA62C1D6;

        }

        e = d;

        d = c;

        c = rol(b, 30);

        b = a;

        a = t;

    }

#else

    for (i = 0; i < 15; i += 5) {

        R0(a, b, c, d, e, 0 + i);

        R0(e, a, b, c, d, 1 + i);

        R0(d, e, a, b, c, 2 + i);

        R0(c, d, e, a, b, 3 + i);

        R0(b, c, d, e, a, 4 + i);

    }

    R0(a, b, c, d, e, 15);

    R1(e, a, b, c, d, 16);

    R1(d, e, a, b, c, 17);

    R1(c, d, e, a, b, 18);

    R1(b, c, d, e, a, 19);

    for (i = 20; i < 40; i += 5) {

        R2(a, b, c, d, e, 0 + i);

        R2(e, a, b, c, d, 1 + i);

        R2(d, e, a, b, c, 2 + i);

        R2(c, d, e, a, b, 3 + i);

        R2(b, c, d, e, a, 4 + i);

    }

    for (; i < 60; i += 5) {

        R3(a, b, c, d, e, 0 + i);

        R3(e, a, b, c, d, 1 + i);

        R3(d, e, a, b, c, 2 + i);

        R3(c, d, e, a, b, 3 + i);

        R3(b, c, d, e, a, 4 + i);

    }

    for (; i < 80; i += 5) {

        R4(a, b, c, d, e, 0 + i);

        R4(e, a, b, c, d, 1 + i);

        R4(d, e, a, b, c, 2 + i);

        R4(c, d, e, a, b, 3 + i);

        R4(b, c, d, e, a, 4 + i);

    }

#endif

    state[0] += a;

    state[1] += b;

    state[2] += c;

    state[3] += d;

    state[4] += e;

}
