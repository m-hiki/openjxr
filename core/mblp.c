#include <stdint.h>
#include "bitbuf.h"
#include "common.h"
#include "enc.h"

#define ABS_LEVEL_IND_LP0   0//  2
#define ABS_LEVEL_IND_LP1   1//  3
#define FIRST_IND_LP_LUM    2//  6
#define FIRST_IND_LP_CHR    3//  7
#define IND_LP_LUM0         4// 10
#define IND_LP_LUM1         5// 11
#define IND_LP_CHR0         6// 12
#define IND_LP_CHR1         7// 13



void init_mblp_param(mblp_param_t *param)
{
    int i;
    
    for (i = 0; i < 15; i++) {
        param->scan.order[i] = SCAN_ORDER0[i];
        param->scan.totals[i] = SCAN_TOTALS[i];
    }
    
    init_model_mb(&param->model, BAND_LP);
    init_vlc_table2(&param->vlc[FIRST_IND_LP_LUM]);
    init_vlc_table2(&param->vlc[IND_LP_LUM0]);
    init_vlc_table2(&param->vlc[IND_LP_LUM1]);
    init_vlc_table2(&param->vlc[FIRST_IND_LP_CHR]);
    init_vlc_table2(&param->vlc[IND_LP_CHR0]);
    init_vlc_table2(&param->vlc[IND_LP_CHR1]);
    init_vlc_table1(&param->vlc[ABS_LEVEL_IND_LP0]);
    init_vlc_table1(&param->vlc[ABS_LEVEL_IND_LP1]);
    param->cbplp.count_zero = 1;
    param->cbplp.count_max = 1;
}


void adapt_lp(adapt_vlc_t *vlc)
{
    adapt_vlc_table2(&vlc[FIRST_IND_LP_LUM], 4);
    adapt_vlc_table2(&vlc[IND_LP_LUM0], 3);
    adapt_vlc_table2(&vlc[IND_LP_LUM1], 3);
    adapt_vlc_table2(&vlc[FIRST_IND_LP_CHR], 4);
    adapt_vlc_table2(&vlc[IND_LP_CHR0], 3);
    adapt_vlc_table2(&vlc[IND_LP_CHR1], 3);
    adapt_vlc_table1(&vlc[ABS_LEVEL_IND_LP0]);
    adapt_vlc_table1(&vlc[ABS_LEVEL_IND_LP1]);
}


void reset_totals_adapt_scan_lp(scan_t *scan)
{
    int i;

    for (i = 0; i < 15; i++) {
        scan->totals[i] = SCAN_TOTALS[i];
    }
}


static void update_count_cbplp(cbplp_t *cbplp, int cbp, int max) {
    cbplp->count_zero += 1 - ((cbp == 0) << 2);
    cbplp->count_zero = clip(cbplp->count_zero, -8, 7);
    cbplp->count_max += 1 - ((cbp == max) << 2);
    cbplp->count_max = clip(cbplp->count_max, -8, 7);
}


static const vlc_t CBPLP_YUV1[2][8] = {
    { // INTERNAL_CLR_FMT = YUV444
        {0x0, 1}, // 0
        {0x4, 3}, // 100
        {0xa, 4}, // 1000
        {0xb, 4}, // 1001
        {0xc, 4}, // 1100
        {0xd, 4}, // 1101
        {0xe, 4}, // 1110
        {0xf, 4}  // 1111
    },
    { // INTERNAL_CLR_FMT = YUV420/YUV422
        {0x0, 1}, // 0
        {0x2, 2}, // 10
        {0x6, 3}, // 110
        {0x7, 3}  // 111
    }
};


const uint8_t REMAP422[7] = {4, 1, 2, 3, 5, 6, 7};


static int fwd_fixed_lp_scan_uv(int *runs, int *levels, int *lp_u, int *lp_v,
                                color_t *color, int model_bits)
{
    int num = color->is_yuv420 ? 3 : 7;
    int v0, v1;
    int i, j;
    
    j = 0;
    runs[j] = 0;
    for (i = 0; i < num; i++) {
        if (color->is_yuv420) {
            v0 = normalize(lp_u[TRANSPOSE420[i + 1]], model_bits);
            v1 = normalize(lp_v[TRANSPOSE420[i + 1]], model_bits);
        } else if (color->is_yuv422) {
            v0 = normalize(lp_u[TRANSPOSE422[REMAP422[i]]], model_bits);
            v1 = normalize(lp_v[TRANSPOSE422[REMAP422[i]]], model_bits);
        }
        
        if (v0 != 0) {
            levels[j] = v0;
            j++;
            runs[j] = 0;
        } else {
            runs[j]++;
        }
        
        if (v1 != 0) {
            levels[j] = v1;
            j++;
            runs[j] = 0;
        } else {
            runs[j]++;
        }
    }
    runs[j] = 0;

    return j;
}


static int enc_cbplp(bitbuf_t *b, model_t *model, color_t *color,
                     cbplp_t *cbplp, int full_planes, int **dclp)
{
    int cbp = 0;
    int n;

    if (color->is_yuv4xx) {
        int num = color->is_yuv444 ? 16 : (color->is_yuv422 ? 8 : 4);
        int max = full_planes * 4 - 5; // Maxvalue of CBPLP
        
        cbp |= (num_nonzero(dclp[0], 16, model->bits[0]) > 0);
        cbp |= (num_nonzero(dclp[1], num, model->bits[1]) > 0) << 1;
        cbp |= (num_nonzero(dclp[2], num, model->bits[1]) > 0) <<
            (2 >> color->is_yuv42x);

        if (cbplp->count_zero <= 0 || cbplp->count_max < 0) {
            int c = cbp;
            if (cbplp->count_max < cbplp->count_zero) {
                c = max - cbp;
            }
            write_e(b, CBPLP_YUV1[color->is_yuv42x][c]);
        } else {
            write_u(b, cbp, full_planes);//CBPLP_YUV2
        }
        update_count_cbplp(cbplp, cbp, max);
    } else {
        for (n = 0; n < color->num_comp; n++) {
            if (num_nonzero(dclp[n], 15, model->bits[n > 0])) {
                write_u1(b, 1);
                cbp |= 1 << n;
            } else {
                write_u1(b, 0);
            }
        }
    }
    
    return cbp;
}


void enc_mb_lp(bitbuf_t *b, mblp_param_t *param, color_t *color, int **dclp)
{
    model_t *model = &param->model;
    adapt_vlc_t *vlc = param->vlc;
    cbplp_t *cbplp = &param->cbplp;
    scan_t *scan = &param->scan;
    int lap_mean[2] = {0, 0};
    int full_planes = color->is_yuv42x ? 2 : color->num_comp;
    int cbp;
    int n, i;
    
    cbp = enc_cbplp(b, model, color, cbplp, full_planes, dclp);

    for (n = 0; n < full_planes; n++) {
        int is_chroma = n > 0;
        int model_bits = model->bits[is_chroma];
        
        if ((cbp >> n) & 1) {//Significant pass
            int runs[16];
            int levels[16];
            int num_nonzero;

            if (is_chroma && color->is_yuv42x) {
                //int num = color->is_yuv420 ? 4 : 8;
                num_nonzero = fwd_fixed_lp_scan_uv(runs, levels, dclp[1], dclp[2], color, model_bits);
            } else {
                num_nonzero = fwd_adapt_scan(runs, levels, dclp[n], scan, model_bits);
            }

            enc_block(b, vlc, BAND_LP, is_chroma, runs, levels, num_nonzero, color);
            lap_mean[is_chroma] += num_nonzero;
        }

        if (model_bits > 0) {//Refinement pass
            if (is_chroma && color->is_yuv42x) {
                int num = color->is_yuv420 ? 4 : 8;

                for (i = 1; i < num; i++) {
                    int idx = color->is_yuv420 ? TRANSPOSE420[i] : TRANSPOSE422[i];
                    enc_refine(b, dclp[1][idx], model_bits);
                    enc_refine(b, dclp[2][idx], model_bits);
                }
            } else {
                for (i = 1; i < 16; i++) {
                    enc_refine(b, dclp[n][TRANSPOSE444[i]], model_bits);
                }
            }
        }
    }
    update_model_mb(lap_mean, model, BAND_LP, color);
}


void inv_adapt_lp_scan(int lp[15], int v, int i, scan_t *scan)
{
    lp[scan->order[i]] = v;
    scan->totals[i]++;
    
    if ((i > 0) && scan->totals[i] > scan->totals[i - 1]) {
        swap(&(scan->totals[i]), &(scan->totals[i - 1]));
        swap(&(scan->order[i]), &(scan->order[i - 1]));
    }
}


static inline void dec_refine_lp(bitbuf_t *b, int coeff, int model_bits)
{
    
}
