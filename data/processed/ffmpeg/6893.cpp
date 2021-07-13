static void scale_coefs (

    int32_t *dst,

    const int32_t *src,

    int dynrng,

    int len)

{

    int i, shift, round;

    int16_t mul;

    int temp, temp1, temp2, temp3, temp4, temp5, temp6, temp7;



    mul = (dynrng & 0x1f) + 0x20;

    shift = 4 - ((dynrng << 23) >> 28);

    if (shift > 0 ) {

      round = 1 << (shift-1);

      for (i=0; i<len; i+=8) {



          temp = src[i] * mul;

          temp1 = src[i+1] * mul;

          temp = temp + round;

          temp2 = src[i+2] * mul;



          temp1 = temp1 + round;

          dst[i] = temp >> shift;

          temp3 = src[i+3] * mul;

          temp2 = temp2 + round;



          dst[i+1] = temp1 >> shift;

          temp4 = src[i + 4] * mul;

          temp3 = temp3 + round;

          dst[i+2] = temp2 >> shift;



          temp5 = src[i+5] * mul;

          temp4 = temp4 + round;

          dst[i+3] = temp3 >> shift;

          temp6 = src[i+6] * mul;



          dst[i+4] = temp4 >> shift;

          temp5 = temp5 + round;

          temp7 = src[i+7] * mul;

          temp6 = temp6 + round;



          dst[i+5] = temp5 >> shift;

          temp7 = temp7 + round;

          dst[i+6] = temp6 >> shift;

          dst[i+7] = temp7 >> shift;



      }

    } else {

      shift = -shift;

      for (i=0; i<len; i+=8) {



          temp = src[i] * mul;

          temp1 = src[i+1] * mul;

          temp2 = src[i+2] * mul;



          dst[i] = temp << shift;

          temp3 = src[i+3] * mul;



          dst[i+1] = temp1 << shift;

          temp4 = src[i + 4] * mul;

          dst[i+2] = temp2 << shift;



          temp5 = src[i+5] * mul;

          dst[i+3] = temp3 << shift;

          temp6 = src[i+6] * mul;



          dst[i+4] = temp4 << shift;

          temp7 = src[i+7] * mul;



          dst[i+5] = temp5 << shift;

          dst[i+6] = temp6 << shift;

          dst[i+7] = temp7 << shift;



      }

    }

}
