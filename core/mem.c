#include "common.h"
#include "enc.h"


#define COPY(w,h)                                        \
    void copy##w##x##h(int *tar, int tar_step,         \
                       int *org, int org_step)         \
    {                                                    \
        int i, j;                                        \
        for (i = 0; i < (h); i++) {                      \
            for (j = 0; j < (w); j++) {                  \
                tar[j] = org[j];                         \
            }                                            \
            tar += tar_step;                             \
            org += org_step;                             \
        }                                                \
    }

COPY(2,4)
COPY(2,2)

#if 0
COPY(4,4)
#else
static inline void copy4x4(int *tar, int tar_step,
                          int *org, int org_step)
{
    int i;
    for (i = 0; i < 4; i++) {
        __asm__ (
                 "lddqu (%0), %%xmm0\n"
                 "movdqu %%xmm0, (%1)"
                 ::"r"(org), "r"(tar)
                 );
        
        tar += tar_step;
        org += org_step;
   }
}
#endif




void ringbuf_update(ringbuf_t *ringbuf)
{
    ringbuf->p += ringbuf->step;

    if (ringbuf->p - ringbuf->buf >= ringbuf->size) {
        ringbuf->p = ringbuf->buf;
    }
}


void load_mb(int **mb, const uint8_t *image, uint32_t width, 
             int num_comp, int mbx, int mby, rect_t *rect)
{//TODO: Support All BD, extended width and windowing.
    int left = rect->left == EX_S ? -4 : NO_S;
    int right = rect->right == EX_E ? 20 : NO_E;
    int top = rect->top == EX_S ? - 4 : NO_S;
    int bottom = rect->bottom == EX_E ? 20 : NO_E;
    int i, j, n;
   
    image += (mby * 16 + top) * width + mbx * 16 * num_comp;

    for (n = 0; n < num_comp; n++) {
        int *tar = mb[n] + top * BW;
        const uint8_t *org = image + n;

        for (i = top; i < bottom; i++) {
            for (j = left; j < right; j++) {
                tar[j] = org[j * num_comp];
            }
            tar += BW;
            org += width;
        }
    }
}


void store_dclp(ringbuf_t *dclp_buf, int **mb_dclp, color_t *color)
{
    int n;
    
    for (n = 0; n < color->num_comp; n++) {
        //printf("store mb dclp %d, %d\n", mb_dclp[n][0], dclp_buf[n].size);
        if (n > 0 && color->is_yuv420) {
            copy2x2(dclp_buf[n].p, dclp_buf[n].size, mb_dclp[n], DCLPW);
        } else if (n > 0 && color->is_yuv422) {
            copy2x4(dclp_buf[n].p, dclp_buf[n].size, mb_dclp[n], DCLPW);
        } else {
            copy4x4(dclp_buf[n].p, dclp_buf[n].size, mb_dclp[n], DCLPW);
        }
    }
}


void load_pred_dclp(int **pred_dclp, ringbuf_t *dclp_buf, color_t *color)
{
    int n;
    
    for (n = 0; n < color->num_comp; n++) {
        if (n > 0 && color->is_yuv420) {
            copy2x2(pred_dclp[n], 2, dclp_buf[n].p, dclp_buf[n].size);
        } else if (n > 0 && color->is_yuv422) {
            copy2x4(pred_dclp[n], 2, dclp_buf[n].p, dclp_buf[n].size);
        } else {
            copy4x4(pred_dclp[n], 4, dclp_buf[n].p, dclp_buf[n].size);
        }
    }
}


int *get_left_dclp(ringbuf_t *dclp_buf)
{
    if (dclp_buf->p - dclp_buf->buf > 0) {
        return dclp_buf->p - dclp_buf->step;
    } else {
        return dclp_buf->buf + dclp_buf->size - dclp_buf->step;
    }
}


int *get_top_dclp(ringbuf_t *dclp_buf)
{
    if (dclp_buf->p - dclp_buf->buf < dclp_buf->size - dclp_buf->step * 2) {
        return dclp_buf->p + dclp_buf->step * 2;
    } else if (dclp_buf->p - dclp_buf->buf < dclp_buf->size - dclp_buf->step) {
        return dclp_buf->buf;
    } else {
        return dclp_buf->buf + dclp_buf->step;
    }
}


int *get_topleft_dclp(ringbuf_t *dclp_buf)
{
    if (dclp_buf->p - dclp_buf->buf < dclp_buf->size - dclp_buf->step) {
        return dclp_buf->p + dclp_buf->step;
    } else {
        return dclp_buf->buf;
    }
}


void store_param(ringbuf_t *param_buf, int param)
{
    param_buf->p[0] = param;
}


int get_cur_param(ringbuf_t *param_buf)
{
    return param_buf->p[0];
}


int get_left_param(ringbuf_t *param_buf)
{
    if (param_buf->p - param_buf->buf > 0) {
        return param_buf->p[-param_buf->step];
    } else {
        return param_buf->buf[param_buf->size - param_buf->step];
    }
}


int get_top_param(ringbuf_t *param_buf)
{
    if (param_buf->p - param_buf->buf < param_buf->size - 1) {
        return param_buf->p[param_buf->step];
    } else {
        return param_buf->buf[0];
    }
}


void load_hp(int ***hp, int **mb, color_t *color)
{
    int i, j, k, n;
    
    for (n = 0; n < color->num_comp; n++) {
        if (n > 0 && color->is_yuv420) {
            for (k = 0; k < 4; k++) {
                i = ((k >> 1) & 1) << 2;
                j = (k & 1) << 2;
                copy4x4(hp[n][k], 4, mb[n] + i * BW + j, BW);
            }
        } else if (n > 0 && color->is_yuv422) {
            for (k = 0; k < 8; k++) {
                i = ((k >> 1) & 3) << 2;
                j = (k & 1) << 2;
                copy4x4(hp[n][k], 4, mb[n] + i * BW + j, BW);
            }
        } else {
            for (k = 0; k < 16; k++) {
                i = ((k >> 2) & 3) << 2;
                j = (k & 3) << 2;
                copy4x4(hp[n][k], 4, mb[n] + i * BW + j, BW);
            }
        }
    }
}
