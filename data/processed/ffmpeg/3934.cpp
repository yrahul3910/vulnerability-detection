static inline void horizX1Filter(uint8_t *src, int stride, int QP)

{

    int y;

    static uint64_t *lut= NULL;

    if(lut==NULL)

    {

        int i;

        lut = av_malloc(256*8);

        for(i=0; i<256; i++)

        {

            int v= i < 128 ? 2*i : 2*(i-256);

/*

//Simulate 112242211 9-Tap filter

            uint64_t a= (v/16)  & 0xFF;

            uint64_t b= (v/8)   & 0xFF;

            uint64_t c= (v/4)   & 0xFF;

            uint64_t d= (3*v/8) & 0xFF;

*/

//Simulate piecewise linear interpolation

            uint64_t a= (v/16)   & 0xFF;

            uint64_t b= (v*3/16) & 0xFF;

            uint64_t c= (v*5/16) & 0xFF;

            uint64_t d= (7*v/16) & 0xFF;

            uint64_t A= (0x100 - a)&0xFF;

            uint64_t B= (0x100 - b)&0xFF;

            uint64_t C= (0x100 - c)&0xFF;

            uint64_t D= (0x100 - c)&0xFF;



            lut[i]   = (a<<56) | (b<<48) | (c<<40) | (d<<32) |

                       (D<<24) | (C<<16) | (B<<8)  | (A);

            //lut[i] = (v<<32) | (v<<24);

        }

    }



    for(y=0; y<BLOCK_SIZE; y++){

        int a= src[1] - src[2];

        int b= src[3] - src[4];

        int c= src[5] - src[6];



        int d= FFMAX(FFABS(b) - (FFABS(a) + FFABS(c))/2, 0);



        if(d < QP){

            int v = d * FFSIGN(-b);



            src[1] +=v/8;

            src[2] +=v/4;

            src[3] +=3*v/8;

            src[4] -=3*v/8;

            src[5] -=v/4;

            src[6] -=v/8;

        }

        src+=stride;

    }

}
