#include <stdint.h>
#include "common.h"
#include "enc.h"


#define ORWT 4


int get_left_param(ringbuf_t *param_buf);
int get_top_param(ringbuf_t *param_buf);
int *get_left_dclp(ringbuf_t *dclp_buf);
int *get_top_dclp(ringbuf_t *dclp_buf);
int *get_topleft_dclp(ringbuf_t *dclp_buf);


static int calc_predmode_dc(ringbuf_t *dclp_buf, color_t *color, rect_t *edge)
{
    int left, top, topleft;
    int strh, strv;
    
    if (edge->left && edge->top) {
        return 3; // No prediction
    } else if (edge->left) {
        return 1; // Predict from top/
    } else if (edge->top) {
        return 0; // Predict from left
    }
    
    left = *get_left_dclp(&dclp_buf[0]);
    top = *get_top_dclp(&dclp_buf[0]);
    topleft = *get_topleft_dclp(&dclp_buf[0]);

    if (!color->is_yuv) {
        strh = ABS(topleft - left);
        strv = ABS(topleft - top);
    } else {
        int left_u = *get_left_dclp(&dclp_buf[1]);
        int top_u = *get_top_dclp(&dclp_buf[1]);
        int topleft_u = *get_topleft_dclp(&dclp_buf[1]);
        int left_v = *get_left_dclp(&dclp_buf[2]);
        int top_v = *get_top_dclp(&dclp_buf[2]);
        int topleft_v = *get_topleft_dclp(&dclp_buf[2]);
        int scale;
        
        if (color->is_yuv420) {
            scale = 8;
        } else if (color->is_yuv422) {
            scale = 4;
        } else {
            scale = 2;
        }
        
        strh = ABS(topleft - left) * scale + 
            ABS(topleft_u - left_u) +
            ABS(topleft_v - left_v);
        
        strv = ABS(topleft - top) * scale + 
            ABS(topleft_u - top_u) +
            ABS(topleft_v - top_v);
    }
    
    if (strh * ORWT < strv) {
        return 1;
    } else if (strv * ORWT < strh) {
        return 0;
    } else {
        return 2; // Predict from left and top
    }
}


static int calc_predmode_lp(int mode_dc, ringbuf_t *lpqp_idx_buf)
{
    int lpqp = lpqp_idx_buf->p[0];
    int lpqp_left = get_left_param(lpqp_idx_buf);
    int lpqp_top = get_top_param(lpqp_idx_buf);

    if (mode_dc == 0 && lpqp == lpqp_left) {
        return 0; // Predict from left
    } else if (mode_dc == 1 && lpqp == lpqp_top) {
        return 1; // Predict from top
    } else {
        return 2; // No prediction
    }
}


void fwd_pred_dclp(int **pred_dclp, ringbuf_t *dclp_buf,
                   ringbuf_t *lpqp_idx_buf, color_t *color, rect_t *edge)
{
    int mode_dc;
    int mode_lp;
    int *dclp_left;
    int *dclp_top;
    int s, n;
    
    mode_dc = calc_predmode_dc(dclp_buf, color, edge);
    mode_lp = calc_predmode_lp(mode_dc, lpqp_idx_buf);

    //printf("|DC:%d|LP:%d\n", mode_dc, mode_lp);
    for (n = 0; n < color->num_comp; n++) {
        dclp_left = get_left_dclp(&dclp_buf[n]);
        dclp_top = get_top_dclp(&dclp_buf[n]);
        s = dclp_buf[n].size;

        if (mode_dc == 0) {
            pred_dclp[n][0] -= dclp_left[0];
        } else if (mode_dc == 1) {
            pred_dclp[n][0] -= dclp_top[0];
        } else if (mode_dc == 2) {
            int ceil = n > 0 && color->is_yuv42x;
            pred_dclp[n][0] -= (dclp_left[0] + dclp_top[0] + ceil) >> 1;
        }
        
        if (n > 0 && color->is_yuv420) {
            if (mode_lp == 0) {
                pred_dclp[n][2] -= dclp_left[s];
            } else if (mode_lp == 1) {
                pred_dclp[n][1] -= dclp_top[1];
            }
        } else if (n > 0 && color->is_yuv422) {
            if (mode_lp == 0) {
                pred_dclp[n][2] -= dclp_left[s];
                pred_dclp[n][4] -= dclp_left[s * 2];
                pred_dclp[n][6] -= dclp_left[s * 3];
            } else if (mode_lp == 1) {
                pred_dclp[n][1] -= dclp_top[1];
                pred_dclp[n][4] -= dclp_top[s * 2];
                pred_dclp[n][5] -= dclp_top[s * 2 + 1];
            } else if (mode_dc == 1) {// predict from current top
                pred_dclp[n][5] -= pred_dclp[n][1];
            }
        } else {
            if (mode_lp == 0) {
                pred_dclp[n][4] -= dclp_left[s];
                pred_dclp[n][8] -= dclp_left[s * 2];
                pred_dclp[n][12] -= dclp_left[s * 3];
            } else if (mode_lp == 1) {
                pred_dclp[n][1] -= dclp_top[1];
                pred_dclp[n][2] -= dclp_top[2];
                pred_dclp[n][3] -= dclp_top[3];
            }
        }
    }
}



static int calc_predmode_hp(int **mb_dclp, color_t *color)
{
    int strh;
    int strv;
    int n;
    // Calculate HP mode
    strh = ABS(mb_dclp[0][1]) + ABS(mb_dclp[0][2]) + ABS(mb_dclp[0][3]);
    strv = ABS(mb_dclp[0][DCLPW]) + ABS(mb_dclp[0][DCLPW * 2]) + ABS(mb_dclp[0][DCLPW * 3]);

    if (color->is_yuv) {
        for (n = 1; n < 3; n++) {
            strh += ABS(mb_dclp[n][1]);
            
            if (color->is_yuv420) {
                strv += ABS(mb_dclp[n][DCLPW]);
            } else if (color->is_yuv42x) {
                strv += ABS(mb_dclp[n][DCLPW]) + ABS(mb_dclp[n][DCLPW * 3]);
                strh += ABS(mb_dclp[n][DCLPW * 2 + 1]);
            } else {
                strv += ABS(mb_dclp[n][DCLPW]);
            }
        }
    }
    //printf("Calc HP : %d, %d\n", strh, strv);    
    if (strh * ORWT < strv) {
        return 0; // Predict from left
    } else if (strv * ORWT < strh) {
        return 1; // Predict from top
    } else {
        return 2; // no prediction
    }
}


int fwd_pred_hp(int ***hp, int **mb_dclp, color_t *color)
{
    int mode_hp;
    int n, k;
    
    mode_hp = calc_predmode_hp(mb_dclp, color);
    
    //printf("Mode hp : %d\n", mode_hp);
    for (n = 0; n < color->num_comp; n++) {
        if (n > 0 && color->is_yuv42x) {
            
        } else {
            if (mode_hp == 0) {
                for (k = 15; k >= 1; k--) {
                    if ((k & 3) != 0) {
                        hp[n][k][4] -= hp[n][k - 1][4];
                        hp[n][k][8] -= hp[n][k - 1][8];
                        hp[n][k][12] -= hp[n][k - 1][12];
                    }
                }
            } else if (mode_hp == 1) {
                for (k = 15; k >= 4; k--) {
                    hp[n][k][1] -= hp[n][k - 4][1];
                    hp[n][k][2] -= hp[n][k - 4][2];
                    hp[n][k][3] -= hp[n][k - 4][3];
                }
            }
        }
    }
    
    return mode_hp;
}
