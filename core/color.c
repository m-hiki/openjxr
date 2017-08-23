#include <stdint.h>
#include <stdlib.h>
#include "../openjxr.h"
#include "common.h"
#include "enc.h"



#if 0
static inline void sub_bias_add_scale(int *r, int *g, int *b, int bias, int scale)
{
    int bi[4] = {bias, bias, bias, bias};

    __asm__(
            "lddqu (%0), %%xmm0\n" //r
            "lddqu (%1), %%xmm1\n" //g
            "lddqu (%2), %%xmm2\n" //b
            "lddqu (%3), %%xmm4\n" //bi
            //"mov   (%4), %%xmm5\n"  //sc
            "psubd  %%xmm4, %%xmm0\n"
            "psubd  %%xmm4, %%xmm1\n"
            "psubd  %%xmm4, %%xmm2\n"
            //"pslld  %%xmm5,  %%xmm0\n"
            "movdqu %%xmm0, (%0)\n"
            "movdqu %%xmm1, (%1)\n"
            "movdqu %%xmm2, (%2)\n"
            :
            :"r"(r), "r"(g), "r"(b), "r"(bi), "r"(scale)
             //:"eax"
            );
}


static inline void fwd_conv1(int *r, int *g, int *b)
{
    int ones[4] = {1, 1, 1, 1};
    __asm__(
            "lddqu (%0), %%xmm0\n" //r
            "lddqu (%1), %%xmm1\n" //g
            "lddqu (%2), %%xmm2\n" //b
            "lddqu (%3), %%xmm4\n" //1
            "psubd  %%xmm0, %%xmm2\n" // b-r
            "movdqa %%xmm2, %%xmm3\n" // v
            "psubd  %%xmm1, %%xmm0\n" // r-g
            "paddd  %%xmm4, %%xmm3\n" // v+1
            "psrad  $1, %%xmm3\n"     // v >>= 1
            "paddd  %%xmm3, %%xmm0\n" // t
            "movdqa %%xmm0, %%xmm3\n" // 
            "psrad  $1, %%xmm3\n"     // t >>=1
            "paddd  %%xmm3, %%xmm1\n" // g + t
            "pxor   %%xmm4, %%xmm4\n" // xmm4 = 0
            "psubd  %%xmm0, %%xmm4\n"
            "movdqu %%xmm1, (%0)\n"
            "movdqu %%xmm4, (%1)\n"
            "movdqu %%xmm2, (%2)\n"
            :
            :"r"(r), "r"(g), "r"(b), "r"(ones)
            );
}
#define STEP 4
#else
static inline void sub_bias_add_scale(int *r, int *g, int *b, int bias, int scale)
{
    *r = (*r - bias) << scale;
    *g = (*g - bias) << scale;
    *b = (*b - bias) << scale;
}


static inline void fwd_conv1(int *r, int *g, int *b)
{
    int v = *b - *r;
    int t = *r - *g + ((v + 1) >> 1);
    int y = *g + (t >> 1);
    int u = -t;
    *r = y;
    *g = u;
    *b = v;
}
#define STEP 1
#endif


static inline void fwd_conv2(int *c, int *m, int *y, int *k)
{
    /*
    *v = c - y;
    *u = c - m + (*v >> 1);
    *yy = k - m + (*u >> 1);
    *kk = k - (*yy >> 1);    
    */
}


static inline void fwd_conv3(int *c, int *m, int *y, int *k)
{
    int t;
    t = *c;
    *c = *k;
    *k = *y;
    *y = *m;
    *m = t;
}


int get_color_conv_idx(int ex_color_format, int ex_bitdepth,
                       int in_color_format)
{
    if (in_color_format == YUVK && ex_color_format == CMYK) {
        return 3;
    } else if (in_color_format == YUVK && ex_color_format == CMYKDIRECT) {
        return 4;
    } else if ((in_color_format == YONLY) &&
               (ex_color_format == RGB &&
                (ex_bitdepth == BD5  || ex_bitdepth == BD565 ||
                 ex_bitdepth == BD10 || ex_bitdepth == BD8 ||
                 ex_bitdepth == BD16 || ex_bitdepth == BD16S ||
                 ex_bitdepth == BD32S))) {
        return 1;
    } else if ((in_color_format == YUV444 || in_color_format == YUV422 ||
                in_color_format == YUV420) && 
               (ex_color_format == RGB &&
                (ex_bitdepth == BD5  || ex_bitdepth == BD565 ||
                 ex_bitdepth == BD10 || ex_bitdepth == BD8 ||
                 ex_bitdepth == BD16 || ex_bitdepth == BD16S))) {
        return 2;
    } else if (in_color_format == YUV444 &&
               (ex_color_format == RGB || ex_color_format == RGBE)) {
        return 2;
    } else {
        return 0;
    }
}


int get_bias(int ex_bitdepth)
{
    if (ex_bitdepth == BD1WHITE1) {
        return 0;
    } else if (ex_bitdepth == BD8) {
        return (1 << 7);
    } else if (ex_bitdepth == BD16) {  
        return (1 << 15);
    } else if (ex_bitdepth == BD5) {
        return (1 << 4);
    } else if (ex_bitdepth == BD10) {
        return (1 << 9);
    } else if (ex_bitdepth == BD565) {
        return (1 << 5);
    } else {
        return 0;
    }
}


// For BD16 BD16S, BD32S
static inline int pre_scaling1(int x, int shift_bits)
{
    return x >> shift_bits;
}

// For BD16F
static inline int pre_scaling_bd16f(int16_t fv)
{
    int is = fv < 0 ? 1 : 0;
    return (fv & 0x7fff) ^ is - is;
}

// For BD32F
static inline int pre_scaling_bd32f(int fv)
{
    int ix;
    
    if (fv == 0) {
        ix = 0;
    } else {
        int is = (fv >> 31) & 0x00000001;
        int ie = (fv >> 23) & 0x000000FF;
        int im = (fv & 0x007FFFFF) | 0x800000;
        
        if (ie == 0) {
            im ^= 0x800000;
            ie++;
        }
        
        //ie = ie - 127 + EXP_BIAS
        if (ie <= 1) {
            
        }
    }

    return ix;
}


void fwd_color_conv(int **mb, rect_t *rect, color_t *color)
{
    int left = rect->left;
    int right = rect->right;
    int top = rect->top;
    int bottom = rect->bottom;
    int bias = color->bias;
    int scale = color->scale;
    int i, j;
    
    switch (color->conv_idx) {
    case 0: // no conversion
        break;
    case 1: // RGB to YONLY
    case 2: // RGB to YUV
        if (color->is_yuv42x) {
            left = left == EX_S ? -4 : NO_S;
            right = right == EX_E ? 20 : NO_E;
            
            if (color->is_yuv420) {
                top = top == EX_S ? - 4 : NO_S;
                bottom = bottom == EX_E ? 20 : NO_E;
            }
        }

        for (i = top; i < bottom; i++) {
            for (j = left; j < right; j += STEP) {
                sub_bias_add_scale(mb[0] + i * BW + j, mb[1] + i * BW + j,
                                   mb[2] + i * BW + j, bias, scale);
                fwd_conv1(mb[0] + i * BW + j, mb[1] + i * BW + j, mb[2] + i * BW + j);
            }
        }
        
        if (color->is_yuv42x) {
            int vs = 1 << color->is_yuv420;
            
            for (i = top; i < bottom; i += vs) {
                for (j = left; j < right; j += 2) {
                    mb[1][(i >> color->is_yuv420) * BW + (j >> 1)] = mb[1][i * BW + j];
                    mb[2][(i >> color->is_yuv420) * BW + (j >> 1)] = mb[2][i * BW + j];
                }
            }
        }
        break;
    case 3: // CMYK to YUVK
        for (i = top; i < bottom; i++) {
            for (j = left; j < right; j++) {
                fwd_conv2(mb[0] + i * BW + j, mb[1] + i * BW + j, 
                          mb[2] + i * BW + j, mb[2] + i * BW + j);
            }
        }
        break;
    case 4:  // CMYK Direct
        for (i = top; i < bottom; i++) {
            for (j = left; j < right; j++) {
                fwd_conv3(mb[0] + i * BW + j, mb[1] + i * BW + j, 
                          mb[2] + i * BW + j, mb[2] + i * BW + j);
            }
        }
        break;
    }
}


void add_bias()
{
    /*
 iScale = 0 // a scale factor when SCALED_FLAG is equal to TRUE
 if (SCALED_FLAG)  
  iScale = 3  
 if (OUTPUT_BITDEPTH = = BD1WHITE1)  
  iBias = 0  
 else if (OUTPUT_BITDEPTH = = BD8)  
  iBias = (1 << 7)  
 else if (OUTPUT_BITDEPTH = = BD16)  
  iBias = (1 << 15)  
 else if (OUTPUT_BITDEPTH = = BD5)  
  iBias = (1 << 4)  
 else if (OUTPUT_BITDEPTH = = BD10)  
  iBias = (1 << 9)  
 else if (OUTPUT_BITDEPTH = = BD565)  
  iBias = (1 << 5)  
 else  
  iBias = 0  
 if ((OUTPUT_BITDEPTH = = BD16) | | (OUTPUT_BITDEPTH = = BD16S) | |  
  (OUTPUT_BITDEPTH = = BD32S))  
  iBias = (iBias >> SHIFT_BITS)  
 if (OUTPUT_CLR_FMT = = RGB) {  
  ImagePlane[0][valXPos][valYPos] += (iBias << iScale) // r
  ImagePlane[1][valXPos][valYPos] += (iBias << iScale) // g
  ImagePlane[2][valXPos][valYPos] += (iBias << iScale) // b
 } else if (OUTPUT_CLR_FMT = = YUV420 | |  
  OUTPUT_CLR_FMT = = YUV422 | | OUTPUT_CLR_FMT = = YUV444) {  
     ImagePlane[0][valXPos][valYPos] += (iBias << iScale) // y
      ImagePlane[1][valXPos][valYPos] += (iBias << iScale) // u
      ImagePlane[2][valXPos][valYPos] += (iBias << iScale) // v
 } else if (OUTPUT_CLR_FMT = = YONLY)  
     ImagePlane[0][valXPos][valYPos] += (iBias << iScale) // y
 else if (OUTPUT_CLR_FMT = = NCOMPONENT | | 
  OUTPUT_CLR_FMT = = CMYKDIRECT)  
  for (i = 0; i<= iComponent; i++)  
      ImagePlane[i][valXPos][valYPos] += (iBias << iScale) // Component i
 else if ((OUTPUT_CLR_FMT = = CMYK) &&  
  ((OUTPUT_BITDEPTH = = BD8) | | (OUTPUT_BITDEPTH = = BD16))) {  
     // output format CMYK
     ImagePlane[0][valXPos][valYPos] += ((iBias >> 1) << iScale) // c
         ImagePlane[1][valXPos][valYPos] += ((iBias >> 1) << iScale) // m
         ImagePlane[2][valXPos][valYPos] += ((iBias >> 1) << iScale) // y
         ImagePlane[3][valXPos][valYPos] −= ((iBias >> 1) << iScale) // k
 }  
}  
 */
}


void upsample()
{
    /*
         chroma
      Centering | iH[0] | iH[1] | iH[2] | iH[3]
              0 |    4  |    4  |    0  |    8
              1 |    5  |    3  |    1  |    7
              2 |    6  |    2  |    2  |    6
              3 |    7  |    1  |    3  |    5
              4 |    8  |    0  |    4  |    4             
      Odd  : y(2k + 1) = (h0 * x(k) + h1 * x(k + 1) + 4) >> 3
      Even : y(2k + 2) = (h2 * x(k) + h3 * x(k + 1) + 4) >> 3
      
      | 0   1   2   3    |    | 0 1 2 3 4 5 6 7 |
      | x . x . x . x .  | => | y y y y y y y y |
      
      y0 = x0,  y1 = 4x0 + 4x1, y2 = x1
      
      for (k = 0; k <= (upsampledLength − 2) / 2; k++)  
          iIntArray[2k+1] = ((iH[0] * iOriArray[k] + iH[1] * 
              iOriArray[Min(upsampledLength / 2 − 1, k+1)] + 4) >> 3)  
      
      for (k = −1; k <= (upsampledLength − 4) / 2; k++)  
          iIntArray[2k+2] = ((iH[2] * iOriArray[Max(0, k)] + 
              iH[3] * iOriArray[k+1] + 4) >> 3)  
    */
}


void compute_scaling()
{
    
}


void inverse_conv_color(uint8_t *image, int step,
                        int **mb, rect_t *ex, int color_conv)
{
    /*
      OutputFormatting() {
          SamplingConversion()
          ConvertInternalToOutputClrFmt()
          AddBias()
          ComputeScaling()
          PostScalingProcess()
          ClippingAndPackingStage()
      }
     */
}

