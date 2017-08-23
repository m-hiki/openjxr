#include <stdint.h>
#include "common.h"
#include "enc.h"

/*
  QP
   0~ 4 : 1 : Lossless
   5~ 8 : 2 : 1/2, 1bit
   9~12 : 3 : 1/3, 2bit
  13~16 : 4 : 1/4, 2bit
  17~20 : 5 : 1/5, 3bit
  21~24 : 6 : 1/6, 3bit
  25~28 : 7 : 1/7, 3bit
  29~32 : 8 : 1/8, 3bit
  33~34 : 9 : 1/9, 4bit
  35~36 :10 : 1/10, 4bit
 */

/*
  Reciprocal Number of Mantissa of Scaling Factor
  Fixed Point Q32 Format
 */
static const uint32_t QP_MAN_RECIP[32][2] = {
    {0x0, 0},       // 0, lossless
    {0x0, 0},       // 1, 
    {0x0, 1},       // 2, 1/2
    {0xaaaaaaab, 1},// 3, 1/3  = 1/1.5    * 1/2
    {0x0, 2},       // 4, 1/4
    {0xcccccccd, 2},// 5, 1/5  = 1/1.25   * 1/4
    {0xaaaaaaab, 2},// 6, 1/6  = 1/1.5    * 1/4
    {0x92492493, 2},// 7, 1/7  = 1/1.75   * 1/4
    {0x0, 3},       // 8, 1/8
    {0xe38e38e4, 3},// 9, 1/9  = 1/1.125  * 1/8
    {0xcccccccd, 3},//10, 1/10 = 1/1.25   * 1/8
    {0xba2e8ba3, 3},//11, 1/11 = 1/1.375  * 1/8
    {0xaaaaaaab, 3},//12, 1/12 = 1/1.5    * 1/8
    {0x9d89d89e, 3},//13, 1/13 = 1/1.625  * 1/8
    {0x92492493, 3},//14, 1/14 = 1/1.75   * 1/8
    {0x88888889, 3},//15, 1/15 = 1/1.875  * 1/8
    {0x0, 4},       //16, 1/16
    {0xf0f0f0f1, 4},//17, 1/17 = 1/1.0625 * 1/16
    {0xe38e38e4, 4},//18, 1/18 = 1/1.125  * 1/16 
    {0xd79435e6, 4},//19, 1/19 = 1/1.1875 * 1/16
    {0xcccccccd, 4},//20, 1/20 = 1/1.25   * 1/16
    {0xc30c30c4, 4},//21, 1/21 = 1/1.3125 * 1/16
    {0xba2e8ba3, 4},//22, 1/22 = 1/1.375  * 1/16
    {0xb21642c9, 4},//23, 1/23 = 1/1.4375 * 1/16
    {0xaaaaaaab, 4},//24, 1/24 = 1/1.5    * 1/16
    {0xa3d70a3e, 4},//25, 1/25 = 1/1.5625 * 1/16
    {0x9d89d89e, 4},//26, 1/26 = 1/1.625  * 1/16
    {0x97b425ee, 4},//27, 1/27 = 1/1.6875 * 1/16
    {0x92492493, 4},//28, 1/28 = 1/1.75   * 1/16
    {0x8d3dcb09, 4},//29, 1/29 = 1/1.8125 * 1/16
    {0x88888889, 4},//30, 1/30 = 1/1.875  * 1/16
    {0x84210843, 4},//31, 1/31 = 1/1.9375 * 1/16
};

//QuantMap
void calc_quant_manexp(uint32_t manexp[2], uint8_t qp, int scaled_flag, int scaled_shift) {
    if (qp == 0) {
        manexp[0] = 0;
        manexp[1] = 0;
    } else {
        uint32_t man;
        uint32_t exp;
        
        if (!scaled_flag) {
            int not_scaled_shift = -2;
            
            if (qp < 32) {
                man = (qp + 3) >> 2;
                exp = 0;
            } else if (qp < 48) {
                man = (16 + (qp % 16) + 1) >> 1;
                exp = (qp >> 4) + not_scaled_shift;
            } else {
                man = 16 + (qp % 16);
                exp = (qp >> 4) - 1 + not_scaled_shift;
            }
        } else {
            if (qp < 16) {
                man = qp;
                exp = scaled_shift;
            } else {
                man = 16 + (qp % 16);
                exp = ((qp >> 4) - 1) + scaled_shift;
            }
        }
        manexp[0] = QP_MAN_RECIP[man][0];
        manexp[1] = exp + QP_MAN_RECIP[man][1];
    }
}


static inline int quant(int coeff, uint32_t man, uint32_t exp)
{//Reciprocal Number & Fixed Point multiplication
    int sign = coeff < 0;
    uint32_t abslv = ABS(coeff);

    if (man == 0) {
        abslv >>= exp;
    } else {
        abslv = (uint32_t)((uint64_t)abslv * man >> (32 + exp));
    }

    coeff = abslv;

    if (sign) {
        coeff = -abslv;
    }
    
    return coeff;
}


void quantize_dclp(int **mb_dclp, uint32_t dc_manexp[2], uint32_t lp_manexp[2], color_t *color)
{
    int dc;
    int i, j, n;

    for (n = 0; n < color->num_comp; n++) {
        int w = 4 >> (color->is_yuv42x && n > 0);
        int h = 4 >> (color->is_yuv420 && n > 0);
        dc = quant(mb_dclp[n][0], dc_manexp[0], dc_manexp[1]);
        
        for (i = 0; i < h; i++) {
            for (j = 0; j < w; j++) {
                mb_dclp[n][i * DCLPW + j] = 
                    quant(mb_dclp[n][i * DCLPW + j], lp_manexp[0], lp_manexp[1]);
            }
        }
        
        mb_dclp[n][0] = dc;
    }
}


void quantize_hp(int **mb, int hpqp[2], color_t *color)
{
    int i, j, n;
    
    for (n = 0; n < color->num_comp; n++) {
        int w = 16 >> (color->is_yuv42x && n > 0);
        int h = 16 >> (color->is_yuv420 && n > 0);

        for (i = 0; i < 16; i++) {
            for (j = 0; j < 16; j++) {
                mb[n][i * BW + j] = quant(mb[n][i * BW + j], hpqp[0], hpqp[1]);
            }
        }
    }
}

