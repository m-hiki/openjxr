#include <stdint.h>
#include "bitbuf.h"
#include "common.h"
#include "enc.h"
#include "dec.h"

#define ABS_LEVEL_IND_DC_LUM  0
#define ABS_LEVEL_IND_DC_CHR  1
#define ABS_LEVEL_DC_VLC(is_chroma) &vlc[(is_chroma)] // 0/1


void init_vlc_table1(adapt_vlc_t *vlc);
void adapt_vlc_table1(adapt_vlc_t *vlc);
void update_model_mb(int lap_mean[2], model_t *model, int band, color_t *color);


void init_mbdc_param(mbdc_param_t *param)
{
    init_model_mb(&param->model, BAND_DC);
    init_vlc_table1(&param->vlc[ABS_LEVEL_IND_DC_LUM]);
    init_vlc_table1(&param->vlc[ABS_LEVEL_IND_DC_CHR]);
}


void adapt_dc(adapt_vlc_t *vlc)
{
    adapt_vlc_table1(&vlc[ABS_LEVEL_IND_DC_LUM]);
    adapt_vlc_table1(&vlc[ABS_LEVEL_IND_DC_CHR]);
}


static const vlc_t VAL_DC_YUV[8] = {
    {0x2, 2}, // 10
    {0x1, 3}, // 001
    {0x1, 5}, // 0000 1
    {0x1, 4}, // 0001
    {0x3, 2}, // 11
    {0x2, 3}, // 010
    {0x0, 5}, // 0000 0
    {0x3, 3}  // 011
};


static void enc_dc(bitbuf_t *b, adapt_vlc_t *vlc, int model_bits, int dc)
{
    int sign = 0;
    int is_nonzero = dc != 0;
    uint32_t dc_ref;
    
    if (dc < 0) {
        sign = 1;
        dc = -dc;
    }
    
    dc_ref = dc & ((1 << model_bits) - 1);
    dc >>= model_bits;
    
    if (dc != 0) enc_abs_level(b, vlc, dc + 1);
    if (model_bits) write_u(b, dc_ref, model_bits);
    if (is_nonzero) write_u1(b, sign);
}


void enc_mb_dc(bitbuf_t *b, mbdc_param_t *param, color_t *color, int **dclp)
{
    model_t *model = &param->model;
    adapt_vlc_t *vlc = param->vlc;
    int lap_mean[2] = {0, 0};
    int n;
    
    if (!color->is_yuv4xx) {
        for (n = 0; n < color->num_comp; n++) {
            int v = ABS(dclp[n][0]);
            int m = n > 0;
            
            if (v >> model->bits[m] != 0) {
                write_u1(b, 1);
                lap_mean[m]++;
            }
            
            enc_dc(b, ABS_LEVEL_DC_VLC(0), model->bits[m], dclp[n][0]);
        }
    } else {
        int val_dc_yuv = 0;

        for (n = 0; n < 3; n++) {
            int v = ABS(dclp[n][0]);
            int m = n > 0;

            if (v >> model->bits[m] != 0) {
                val_dc_yuv |= 4 >> n;
                lap_mean[m]++;
            } 
        }
        //printf("VAL_DC_YUV : %d\n", val_dc_yuv);

        write_e(b, VAL_DC_YUV[val_dc_yuv]);
        enc_dc(b, ABS_LEVEL_DC_VLC(0), model->bits[0], dclp[0][0]);
        enc_dc(b, ABS_LEVEL_DC_VLC(1), model->bits[1], dclp[1][0]);
        enc_dc(b, ABS_LEVEL_DC_VLC(1), model->bits[1], dclp[2][0]);
    }
    
    update_model_mb(lap_mean, &model[BAND_DC], BAND_DC, color);
}


static int dec_dc(bitbuf_t *b, adapt_vlc_t *vlc, 
                    int model_bits, int b_abs_level)
{
    int32_t dc = 0;
    
    if (b_abs_level) dc = dec_abs_level(b, vlc) - 1;
    if (model_bits) dc = (dc << model_bits) | read_u(b, model_bits);
    if (dc != 0) {
        if (read_u1(b)) dc = -dc;
    }

    return dc;
}


void dec_mb_dc(bitbuf_t *b, mbdc_param_t *param, color_t *color, int32_t **dc)
{
    model_t *model = &param->model;
    adapt_vlc_t *vlc = param->vlc;
    int lap_mean[2] = {0, 0};
    int n;

    if (!color->is_yuv4xx) {
        for (n = 0; n < color->num_comp; n++) {
            int b_abs_level = read_u1(b);
            int m = n > 0;

            if (b_abs_level) lap_mean[m]++;
            dc[n][0] = dec_dc(b, ABS_LEVEL_DC_VLC(0), model->bits[m], b_abs_level);
        }
    } else {
        int val_dc_yuv = read_e(b, VAL_DC_YUV, 8);
        int b_abs_level;
        printf("VAL_DC_YUV : %d\n", val_dc_yuv);
        b_abs_level = val_dc_yuv & 4;
        if (b_abs_level) lap_mean[0]++;
        dc[0][0] = dec_dc(b, ABS_LEVEL_DC_VLC(0), model->bits[0], b_abs_level);
        b_abs_level = val_dc_yuv & 2;
        if (b_abs_level) lap_mean[1]++;
        dc[1][0] = dec_dc(b, ABS_LEVEL_DC_VLC(1), model->bits[1], b_abs_level);
        b_abs_level = val_dc_yuv & 1;
        if (b_abs_level) lap_mean[1]++;
        dc[2][0] = dec_dc(b, ABS_LEVEL_DC_VLC(1), model->bits[1], b_abs_level);
    }
}
