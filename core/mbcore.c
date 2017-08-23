#include <stdio.h>
#include <stdint.h>
#include "bitbuf.h"
#include "common.h"
#include "enc.h"
#include "dec.h"


#define ABS_LEVEL_VLC(ctx) &vlc[(ctx)]
#define FIRST_INDEX_VLC(is_chroma) &vlc[(is_chroma) + 2]
#define INDEX_VLC(is_chroma, ctx) &vlc[(((is_chroma) << 1) | (ctx)) + 4]


const uint8_t SCAN_ORDER0[15] = {
    4, 1, 5, 8, 2, 9, 6, 12, 3, 10, 13, 7, 14, 11, 15
};


const uint8_t SCAN_ORDER1[15] = {
    1, 2, 5, 4, 3, 6, 9, 8, 7, 12, 15, 13, 10, 11, 14
};


const uint8_t SCAN_TOTALS[15] = {
    32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4
};


const uint8_t TRANSPOSE444[16] = {
    0,  4,  8, 12,
    1,  5,  9, 13,
    2,  6, 10, 14,
    3,  7, 11, 15
};


const uint8_t TRANSPOSE422[8] = {
    0, 2, 1, 3,
    4, 6, 5, 7};


const uint8_t TRANSPOSE420[4] = {0, 2, 1, 3};


int num_nonzero(int *coeff, int num, int model_bits)
{
    int num_nonzero = 0;
    int i;
    
    for (i = 1; i < num; i++) {
        num_nonzero += (ABS(coeff[i]) >> model_bits) != 0;
    }

    return num_nonzero;
}


int fwd_adapt_scan(int *runs, int *levels, int *coeff, scan_t *scan, int model_bits)
{
    int i, j = 0;
    
    runs[0] = 0;
    for (i = 0; i < 15; i++) {       
        int v = normalize(coeff[scan->order[i]], model_bits);
        
        if (v != 0) {
            scan->totals[i]++;
            
            if ((i > 0) && scan->totals[i] > scan->totals[i - 1]) {
                swap(&scan->totals[i], &scan->totals[i - 1]);
                swap(&scan->order[i], &scan->order[i - 1]);
            }

            levels[j] = v; 
            j++;
            runs[j] = 0;
        } else {
            runs[j]++;
        }
    }

    runs[j] = 0;
    return j;
}


static const vlc_t ABS_LEVEL_INDEX[2][7] = {
    { // Code 0
        {0x1, 2}, // 01
        {0x2, 2}, // 10
        {0x3, 2}, // 11
        {0x1, 3}, // 001
        {0x1, 4}, // 0001
        {0x1, 5}, // 0000 0
        {0x1, 5}, // 0000 1
    },
    { // Code 1
        {0x1, 1}, // 1
        {0x1, 2}, // 01
        {0x1, 3}, // 001
        {0x1, 4}, // 0001
        {0x1, 5}, // 0000 1
        {0x0, 6}, // 0000 00
        {0x1, 6}, // 0000 01
    }
};


static const int8_t ABS_LEVEL_INDEX_DELTA[7] = {
   1, 0, -1, -1, -1, -1, -1
};


static const uint8_t ABS_LEVEL_LIMIT[6] = {2, 3, 5, 9, 13, 17};
static const uint8_t ABS_LEVEL_REMAP[6] = {2, 3, 4, 6, 10, 14};
static const uint8_t ABS_LEVEL_FIXED_LEN[6] = {0, 0, 1, 2, 2, 2};


static inline int nlz(uint32_t x)
{
    uint32_t y;
    int n = 32;

    y = x >> 16; if (y != 0) {n -= 16; x = y;}
	y = x >> 8; if (y != 0) {n -= 8; x = y;}
	y = x >> 4; if (y != 0) {n -= 4; x = y;}
	y = x >> 2; if (y != 0) {n -= 2; x = y;}
	y = x >> 1; if (y != 0) {return 31 - (n - 2);}

 	return 31 - (n - x);
}


void enc_abs_level(bitbuf_t *b, adapt_vlc_t *vlc, uint32_t level)
{
    uint32_t abslv_idx = 0;
    
    while (abslv_idx < 6 && ABS_LEVEL_LIMIT[abslv_idx] < level) {
        abslv_idx++;
    }
    
    write_e(b, ABS_LEVEL_INDEX[vlc->table_index][abslv_idx]);
    vlc->discrim_val1 += ABS_LEVEL_INDEX_DELTA[abslv_idx];
    
    if (abslv_idx < 6) {
        uint32_t fixed = ABS_LEVEL_FIXED_LEN[abslv_idx];
        uint32_t level_ref = level - ABS_LEVEL_REMAP[abslv_idx];
        
        if (fixed > 0) write_u(b, level_ref, fixed);
    } else { // level >= 18
        uint32_t fixed = nlz(level - 2);
        uint32_t tmp = fixed - 4;
        uint32_t level_ref = level - (1 << fixed) - 2;
        
        if (tmp < 15) {
            write_u(b, tmp, 4);
        } else {
            write_u(b, 0xF, 4);
            tmp -= 15;
            
            if (tmp < 3) {
                write_u(b, tmp, 2);
            } else {
                write_u(b, 0x3, 2);
                write_u(b, tmp - 3, 3);
            }
        }

        write_u(b, level_ref, fixed);
     }
}


int dec_abs_level(bitbuf_t *b, adapt_vlc_t *vlc) 
{
    int abslv_idx;
    int lv;
    
    abslv_idx = read_e(b, ABS_LEVEL_INDEX[vlc->table_index], 7);
    vlc->discrim_val1 += ABS_LEVEL_INDEX_DELTA[abslv_idx];

    if (abslv_idx < 6) {
        uint32_t fixed = ABS_LEVEL_FIXED_LEN[abslv_idx];
        lv = ABS_LEVEL_REMAP[abslv_idx];

        if (fixed > 0) lv += read_u(b, fixed);
    } else {
        uint32_t fixed = read_u(b, 4) + 4;

        if (fixed == 19) fixed += read_u(b, 2);
        if (fixed == 22) fixed += read_u(b, 3);
        
        lv += 2 + (1 << fixed) + read_u(b, fixed);
    }

    return lv;
}


static const vlc_t FIRST_INDEX[5][12] = {
    {// Code 0
        {0x1, 5}, // 0000 1
        {0x1, 6}, // 0000 01
        {0x0, 7}, // 0000 000
        {0x1, 7}, // 0000 001
        {0x4, 5}, // 0010 0
        {0x2, 3}, // 010
        {0x5, 5}, // 0010 1
        {0x1, 1}, // 1
        {0x6, 5}, // 0011 0
        {0x1, 4}, // 0001
        {0x7, 5}, // 0011 1
        {0x3, 3}  // 011
    },
    {// Code 1
        {0x2, 4}, // 0010
        {0x2, 5}, // 0001 0
        {0x0, 6}, // 0000 00
        {0x1, 6}, // 0000 01
        {0x3, 4}, // 0011
        {0x2, 3}, // 010
        {0x3, 5}, // 0001 1
        {0x3, 2}, // 11
        {0x3, 3}, // 011
        {0x4, 3}, // 100
        {0x1, 5}, // 0000 1
        {0x5, 3}  // 101
    },
    {// Code 2
        {0x3, 2}, // 11
        {0x1, 3}, // 001
        {0x0, 7}, // 0000 000
        {0x1, 7}, // 0000 001
        {0x1, 5}, // 0000 1
        {0x2, 3}, // 010
        {0x2, 7}, // 0000 010
        {0x3, 3}, // 011
        {0x4, 3}, // 100
        {0x5, 3}, // 101
        {0x3, 7}, // 0000 011
        {0x1, 4}  // 0001
    },
    {// Code 3
        {0x1, 3}, // 001
        {0x3, 2}, // 11
        {0x0, 7}, // 0000 000
        {0x1, 5}, // 0000 1
        {0x2, 5}, // 0001 0
        {0x2, 3}, // 010
        {0x1, 7}, // 0000 001
        {0x3, 3}, // 011
        {0x3, 5}, // 0001 1
        {0x4, 3}, // 100
        {0x1, 6}, // 0000 01
        {0x5, 3}  // 101
    },
    {// Code 4
        {0x2, 3}, // 010
        {0x1, 1}, // 1
        {0x1, 7}, // 0000 001
        {0x1, 4}, // 0001
        {0x2, 7}, // 0000 010
        {0x3, 3}, // 011
        {0x0, 8}, // 0000 0000
        {0x2, 4}, // 0010
        {0x3, 7}, // 0000 011
        {0x3, 4}, // 0011
        {0x1, 8}, // 0000 0001
        {0x1, 5}  // 0000 1
    }
};


static const int8_t FIRST_INDEX_DELTA[4][12] = {
    { 1,  1,  1,  1,  1,  0,  0, -1,  2,  1,  0,  0},
    { 2,  2, -1, -1, -1,  0, -2, -1,  0,  0, -2, -1},
    {-1,  1,  0,  2,  0,  0,  0,  0, -2,  0,  1,  1},
    { 0,  1,  0,  1, -2,  0, -1, -1, -2, -1, -2, -2}
};


static void enc_first_index(bitbuf_t *b, adapt_vlc_t *vlc, int first_index)
{
    write_e(b, FIRST_INDEX[vlc->table_index][first_index]);
    vlc->discrim_val1 += FIRST_INDEX_DELTA[vlc->delta_table_index][first_index];
    vlc->discrim_val2 += FIRST_INDEX_DELTA[vlc->delta2_table_index][first_index];
}


static uint32_t dec_first_index(bitbuf_t *b, adapt_vlc_t *vlc, 
                                int first_index)
{
    return 0;
}


static const int8_t INDEX_1_DELTA[3][6] = {
    {-1, 1, 1, 1, 0, 1},
    {-2, 0, 0, 2, 0, 0},
    {-1,-1, 0, 1,-2, 0}
};


static const vlc_t INDEX_A[4][6] = {
    {// Code 0
        {0x1, 1}, // 1
        {0x0, 5}, // 0 0000
        {0x1, 3}, // 001
        {0x1, 5}, // 0 0001
        {0x1, 2}, // 01
        {0x1, 4}  // 0001
    },
    {// Code 1
        {0x1, 2}, // 01
        {0x0, 4}, // 0000
        {0x2, 2}, // 10
        {0x1, 4}, // 0001
        {0x3, 2}, // 11
        {0x1, 3}  // 001
    },
    {// Code 2
        {0x0, 4}, // 0000
        {0x1, 4}, // 0001
        {0x1, 2}, // 01
        {0x2, 2}, // 10
        {0x3, 2}, // 11
        {0x1, 3}  // 001
    },
    {// Code 3
        {0x0, 5}, // 0 0000
        {0x1, 5}, // 0 0001
        {0x1, 2}, // 01
        {0x1, 1}, // 1
        {0x1, 4}, // 0001
        {0x1, 3}  // 001
    }
};


static const vlc_t INDEX_B[4] = {
    {0x0, 1}, // 0
    {0x6, 3}, // 110
    {0x2, 2}, // 10
    {0x7, 3}  // 111
};


static void enc_index(bitbuf_t *b, adapt_vlc_t *vlc, int location, int index)
{
    if (location < 15) {
        write_e(b, INDEX_A[vlc->table_index][index]);
        vlc->discrim_val1 += INDEX_1_DELTA[vlc->delta_table_index][index];
        vlc->discrim_val2 += INDEX_1_DELTA[vlc->delta2_table_index][index];
    } else if (location == 15) {
        write_e(b, INDEX_B[index]);
    } else {//location == 16
        write_u1(b, 1);
    }
}


static void dec_index(bitbuf_t *b, adapt_vlc_t *vlc, int location, int index)
{

}


static const vlc_t RUN_VALUE[3][4] = {
    {// max_run == 2
        {1, 1}, // 1
        {0, 1}  // 0
    },
    {// max_run == 3
        {1, 1}, // 1
        {1, 2}, // 01
        {0, 2}  // 00
    },
    {// max_run == 4
        {1, 1}, // 1
        {1, 2}, // 01
        {1, 3}, // 001
        {0, 3}  // 000
    }
};


static const vlc_t RUN_INDEX[5] = {
    {0x1, 1}, // 1
    {0x1, 2}, // 01
    {0x1, 3}, // 001
    {0x0, 4}, // 0000
    {0x1, 4}  // 0001
};


static const uint8_t RUN_INDEX_MAP[34] = {
    0, 1, 2, 2, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    0, 1, 2, 2, 3, 3, 4, 4, 4, 4, 0, 0, 0, 0,
    0, 1, 2, 3, 4, 4
};


static const uint8_t RUN_REMAP[15] = {
    1, 2, 3, 5, 7, 1, 2, 3, 5, 7, 1, 2, 3, 4, 5
};


static const int8_t RUN_BIN[15] = {
    -1, -1, -1, -1, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0
};


static const uint8_t RUN_FIXED_LENGTH[15] = {
    0, 0, 1, 1, 3, 0, 0, 1, 1, 2, 0, 0, 0, 0, 1};


static void enc_run(bitbuf_t *b, int max_run, int run)
{
    if (max_run < 5) {
        if (max_run > 1) {//max_run=2~4
            write_e(b, RUN_VALUE[max_run - 2][run - 1]);
        }
    } else {
        int bin = RUN_BIN[max_run];
        uint32_t run_index = RUN_INDEX_MAP[run + bin * 14 - 1];
        uint32_t index = run_index + bin * 5;
        uint32_t fixed = RUN_FIXED_LENGTH[index];

        write_e(b, RUN_INDEX[run_index]);

        if (fixed) {
            int run_ref = run - RUN_REMAP[index];
            write_u(b, run_ref, fixed);
        }
    }
}


int dec_run(bitbuf_t *b, int max_run)
{
    return 0;
}



void enc_block(bitbuf_t *b, adapt_vlc_t *vlc, int band, int is_chroma, 
               int runs[15], int levels[15], int num_nonzero, color_t *color)
{
    int idx;
    int sign;
    int abslev;
    int ctx;
    int location = 1;
    int i;

    if (is_chroma && band == BAND_LP) {
        if (color->is_yuv420) {
            location = 10;
        } else if (color->is_yuv422) {
            location = 2;
        }
    }

    sign = levels[0] < 0;
    abslev = ABS(levels[0]);
    idx = (runs[0] == 0) | ((abslev != 1) << 1);
    if (num_nonzero != 1) idx |= runs[1] == 0 ? 0x4 : 0x8;
    ctx = (idx & 0x1) & (idx >> 2);

    enc_first_index(b, FIRST_INDEX_VLC(is_chroma), idx);
    write_u1(b, sign);
    if (abslev != 1) enc_abs_level(b, ABS_LEVEL_VLC(ctx), abslev);
    if (runs[0] > 0) enc_run(b, 15 - location, runs[0]);

    location += 1 + runs[0];

    for (i = 1; i < num_nonzero; i++) {
        if (runs[i] > 0) enc_run(b, 15 - location, runs[i]);

        location += 1 + runs[i];
        sign = levels[i] < 0;
        abslev = ABS(levels[i]);
        idx = abslev != 1;
        if (i < num_nonzero - 1) idx |= runs[i + 1] == 0 ? 0x2 : 0x4;

        enc_index(b, INDEX_VLC(is_chroma, ctx), location, idx);
        write_u1(b, sign);

        ctx &= idx >> 1;
        if (abslev != 1) enc_abs_level(b, ABS_LEVEL_VLC(ctx), abslev);
    }
}


void dec_block(bitbuf_t *b, adapt_vlc_t *vlc, int band, int is_chroma, 
               int runs[15], int levels[15], int num_non_zero)
{

}


void enc_refine(bitbuf_t *b, int coeff, int model_bits)
{
   if (coeff != 0) {
       int sign = coeff < 0;
       if (sign) coeff = -coeff;
       write_u(b, coeff, model_bits);
       if (coeff >> model_bits == 0) {
           write_u1(b, sign);
       }
   } else {
       write_u(b, 0, model_bits);
   }
}



void init_model_mb(model_t *model, int band)
{
    model->state[0] = model->state[1] = 0;
    model->bits[0] = model->bits[1] = (2 - band) * 4; //DC:0, LP:1, HP:2
}


#define MODEL_WEIGHT 70

static const uint8_t WEIGHT0[3] = {240, 12, 1};


static const uint8_t WEIGHT1[3][MAX_COMPONENTS] = {
    {0, 240, 120, 80, 60, 48, 40, 34, 30, 27, 24, 22, 20, 18, 17, 16},
    {0,  12,   6,  4,  3,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1},
    {0,  16,   8,  5,  4,  3,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1}
};


static const uint8_t WEIGHT2[6] = {120, 37, 2, //YUV420,
                                   120, 18, 1}; //YUV422

void update_model_mb(int lap_mean[2], model_t *model, int band, color_t *color)
{
    int num_models = color->is_yonly ? 1 : 2;
    int i;
    
    lap_mean[0] *= WEIGHT0[band];
    
    if (color->is_yuv420) {
        lap_mean[1] *= WEIGHT2[band];
    } else if (color->is_yuv422) {
        lap_mean[1] *= WEIGHT2[3 + band];
    } else {
        lap_mean[1] *= WEIGHT1[band][color->num_comp - 1];
        
        if (band == BAND_HP) {
            lap_mean[1] >>= 4;
        }
    }

    for (i = 0; i < num_models; i++) {
        int ms = model->state[i];
        int delta = (lap_mean[i] - MODEL_WEIGHT) >> 2;
        
        if (delta <= -8) {
            delta += 4;
            
            if (delta < -16) {
                delta = -16;
            }
            
            ms += delta;
            
            if (ms < -8) {
                if (model->bits[i] == 0) {
                    ms = -8;
                } else {
                    ms = 0;
                    model->bits[i]--;
                }
            }
        } else if (delta >= 8) {
            delta -= 4;
            
            if (delta > 15) {
                delta = 15;
            }
            
            ms += delta;
            
            if (ms > 8) {
                if (model->bits[i] >= 15) {
                    model->bits[i] = 15;
                    ms = 8;
                } else {
                    ms = 0;
                    model->bits[i]++;
                }
            }
        }
        model->state[i] = ms;
    }
}


void init_vlc_table1(adapt_vlc_t *vlc)
{
    vlc->table_index = 0;
    vlc->delta_table_index = 0;
    vlc->discrim_val1 = 0;
}


void init_vlc_table2(adapt_vlc_t *vlc)
{
    vlc->table_index = 1;
    vlc->delta_table_index = 0;
    vlc->delta2_table_index = 1;
    vlc->discrim_val1 = 0;
    vlc->discrim_val2 = 0;
}


#define MAX_TABLE_INDEX  1
#define LOWER_BOUND     -8
#define UPPER_BOUND      8


void adapt_vlc_table1(adapt_vlc_t *vlc)
{
    if (vlc->discrim_val1 < LOWER_BOUND && vlc->table_index != 0) {
        vlc->table_index--;
        vlc->discrim_val1 = 0;
    } else if (vlc->discrim_val1 > UPPER_BOUND && 
               vlc->table_index != MAX_TABLE_INDEX) {
        vlc->table_index++;
        vlc->discrim_val1 = 0;
    } else {
        vlc->discrim_val1 = clip(vlc->discrim_val1, -64, 64);
    }
}


void adapt_vlc_table2(adapt_vlc_t *vlc, int max_table_index)
{
    int discrim_low = vlc->discrim_val1;
    int discrim_high = vlc->discrim_val2;
    int change = 0;
    
    if (discrim_low < LOWER_BOUND && vlc->table_index != 0) {
        vlc->table_index--;
        change = 1;
    } else if (discrim_high > UPPER_BOUND
               && vlc->table_index != max_table_index) {
        vlc->table_index++;
        change = 1;
    }
    
    if (change) {
        vlc->discrim_val1 = 0;
        vlc->discrim_val2 = 0;
        
        if (vlc->table_index == max_table_index) {
            vlc->delta_table_index = vlc->table_index - 1;
            vlc->delta2_table_index = vlc->table_index - 1;
        } else if (vlc->table_index == 0) {
            vlc->delta_table_index = vlc->table_index;
            vlc->delta2_table_index = vlc->table_index;
        } else {
            vlc->delta_table_index = vlc->table_index - 1;
            vlc->delta2_table_index = vlc->table_index;
        }
    } else {
        vlc->discrim_val1 = clip(vlc->discrim_val1, -64, 64);
        vlc->discrim_val2 = clip(vlc->discrim_val2, -64, 64);
    }
}
