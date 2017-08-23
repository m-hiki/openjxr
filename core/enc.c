#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include "../openjxr.h"
#include "common.h"
#include "enc.h"




void enc_alloc(jxr_t *p)
{
    int n, k;

    n = p->color.num_comp;
    p->mb_buf = (int**)malloc(sizeof(int*) * n);
    p->mb = (int**)malloc(sizeof(int*) * n);
    p->mb_dclp_buf = (int**)malloc(sizeof(int*) * n);
    p->mb_dclp = (int**)malloc(sizeof(int*) * n);
    p->pred_dclp = (int**)malloc(sizeof(int*) * n);
    p->hp = (int***)malloc(sizeof(int**) * n);
    p->dclp_buf = (ringbuf_t*)malloc(sizeof(ringbuf_t) * n);
    p->cbphp_buf = (ringbuf_t*)malloc(sizeof(ringbuf_t) * n);
    p->lpqp_idx_buf.buf = (int*)malloc(sizeof(int) * (p->param.mb_width + 1));
    p->lpqp_idx_buf.p = p->lpqp_idx_buf.buf;
    p->lpqp_idx_buf.size = p->param.mb_width + 1;
    p->lpqp_idx_buf.step = 1;

    for (n = 0; n < p->color.num_comp; n++) {
        int w = n > 0 && p->color.is_yuv42x ? 2 : 4;
        int h = n > 0 && p->color.is_yuv420 ? 2 : 4;
        
        p->mb_buf[n] = (int*)malloc(sizeof(int) * BW * BH);
        p->mb[n] = p->mb_buf[n] + 4 + BW * 4;
        p->mb_dclp_buf[n] = (int*)malloc(sizeof(int) * DCLPW * DCLPH);
        p->mb_dclp[n] = p->mb_dclp_buf[n];
        p->pred_dclp[n] = (int*)malloc(sizeof(int) * w * h);
        p->hp[n] = (int**)malloc(sizeof(int*) * w * h);
        for (k = 0; k < w * h; k++) {
            p->hp[n][k] = (int*)malloc(sizeof(int) * 4 * 4);
        }
        
        p->dclp_buf[n].buf = (int*)malloc(sizeof(int) * w * h * (p->param.mb_width + 2));
        p->dclp_buf[n].p = p->dclp_buf[n].buf;        
        p->dclp_buf[n].size = (p->param.mb_width + 2) * w;
        p->dclp_buf[n].step = w;

        p->cbphp_buf[n].buf = (int*)malloc(sizeof(int) * (p->param.mb_width + 1));
        p->cbphp_buf[n].p = p->cbphp_buf[n].buf;
        p->cbphp_buf[n].size = p->param.mb_width + 1;
        p->cbphp_buf[n].step = 1;
    }
    
    if (!get_flag(p->param.flags, FREQUENCY_MODE_CODESTREAM_FLAG)) {
        p->out = (bitbuf_t*)malloc(sizeof(bitbuf_t));
        p->out[0].buf =  (uint32_t*)malloc(sizeof(uint32_t) * p->param.width * p->param.height);
        p->out[0].p = p->out[0].buf;
        p->out[0].left = 32;
        //p->out[0].length = p->param.width * p->param.height * p->color.num_comp;
    } else {
        p->out = (bitbuf_t*)malloc(sizeof(bitbuf_t) * 4);
        for (k = 0; k < 4; k++) {
            p->out[k].buf =  (uint32_t*)malloc(sizeof(uint32_t) * p->param.width * p->param.height);
            p->out[k].p = p->out[k].buf;
            p->out[k].left = 32;
            //p->out[k].length = p->param.width * p->param.height * p->color.num_comp;
        }
    }
    
    p->color.conv_idx = get_color_conv_idx(p->param.ex_color_format,
                                           p->param.ex_bitdepth,
                                           p->param.in_color_format);
    p->color.bias = get_bias(p->param.ex_bitdepth);
    
    if (!get_flag(p->param.flags, TILING_FLAG)) {
        p->param.num_ver_tiles = 1;
        p->param.num_hor_tiles = 1;
        p->param.tile_width_in_mb = (uint16_t*)malloc(sizeof(uint16_t));
        p->param.tile_height_in_mb = (uint16_t*)malloc(sizeof(uint16_t));
        p->param.tile_width_in_mb[0] = p->param.mb_width;
        p->param.tile_height_in_mb[0] = p->param.mb_height;
    }
    
    p->step = p->param.width * n;

    //enc_image_header(p->out, &p->param);
    //enc_image_plane_header(p->out, &p->param);
    p->is_ready = 1;
}


static inline void check_tile_edge(rect_t *is_tile_edge, int mbx, int mby, 
                                   rect_t *tile)
{
    is_tile_edge->left = mbx == tile->left;
    is_tile_edge->right = mbx == tile->right;
    is_tile_edge->top = mby == tile->top;
    is_tile_edge->bottom = mby == tile->bottom;
}


static inline void calc_rect(rect_t rect[2], int mbx, int mby,
                             int width_in_mb, int height_in_mb)
{//TODO : Support Hard tiling flag
    rect[0].left = mbx > 0 ? EX_S : NO_S;
    rect[0].right = mbx < width_in_mb - 1 ? EX_E : NO_E;
    rect[0].top = mby > 0 ? EX_S : NO_S;
	rect[0].bottom = mby < height_in_mb - 1 ? EX_E : NO_E;
    rect[1].left = mbx > 0 ? EX_S : NO_S;
    rect[1].right = mbx < width_in_mb - 1 ? EX_EC : NO_E;
    rect[1].top = mby > 0 ? EX_S : NO_S;
	rect[1].bottom = mby < height_in_mb - 1 ? EX_EC : NO_E;
}


void enc_tile(jxr_t *p, rect_t *tile)
{
    rect_t mb_window[2];
    rect_t is_tile_edge;
    uint32_t mby, mbx;

    enc_tile_header(p->out, &p->param);
    //printf("%d, %d, %d, %d\n", tile->left, tile->right, tile->top, tile->bottom);
    for (mby = tile->top; mby <= tile->bottom; mby++) {
        for (mbx = tile->left; mbx <= tile->right; mbx++) {
            //printf("(%3d, %3d)", mbx, mby);
            
            check_tile_edge(&is_tile_edge, mbx, mby, tile);
            calc_rect(mb_window, mbx, mby, p->param.mb_width, p->param.mb_height);
            //printf("%d, %d, %d, %d\n", mb_window[0].left, mb_window[0].right, mb_window[0].top, mb_window[0].bottom);
            enc_mb_stage1(p, mb_window, &is_tile_edge, mbx, mby);
            enc_mb_stage2(p, tile, &is_tile_edge, mbx, mby);
            //printf("\n");
            //if (mbx == 0 && mby == 1) exit(0);
        }
    }
    
    write_align(&p->out[0]);
    if (get_flag(p->param.flags, FREQUENCY_MODE_CODESTREAM_FLAG)) {
        write_align(&p->out[1]);
        write_align(&p->out[2]);
        write_align(&p->out[3]);
    }

    int mbs = (tile->right - tile->left + 1) * (tile->bottom - tile->top + 1);
    int tb = get_total_bits(p->out);
    //printf("|Total : %d[bits]|MBs : %d| %3.2f[bits/MB]|Comp. Ratio %f|\n", tb, mbs, (float)tb/mbs, (float)tb/(mbs*6144));
}


void enc_free(jxr_t *p)
{
    int n, k;

    if (!get_flag(p->param.flags, FREQUENCY_MODE_CODESTREAM_FLAG)) {
        free(p->out[0].buf);
        free(p->out);
    } else {
        for (k = 0; k < 4; k++) {
            free(p->out[k].buf);
        }
        free(p->out);
    }
    
    for (n = 0; n < p->color.num_comp; n++) {
        int w = n > 0 && p->color.is_yuv42x ? 2 : 4;
        int h = n > 0 && p->color.is_yuv420 ? 2 : 4;

        free(p->cbphp_buf[n].buf);
        free(p->dclp_buf[n].buf);
        for (k = 0; k < w * h; k++) {
            free(p->hp[n][k]);
        }
        free(p->hp[n]);
        free(p->pred_dclp[n]);
        free(p->mb_dclp_buf[n]);
        free(p->mb_buf[n]);
    }
    free(p->lpqp_idx_buf.buf);
    free(p->hp);
    free(p->cbphp_buf);
    free(p->dclp_buf);
    free(p->mb_dclp_buf);
    free(p->mb_dclp);
    free(p->pred_dclp);
    free(p->mb);
    free(p->mb_buf);
    free(p);
}


