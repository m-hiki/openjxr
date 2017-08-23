#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include "enc.h"


void enc_mb_stage1(jxr_t *p, rect_t *mb_window, rect_t *is_tile_edge, uint32_t mbx, uint32_t mby)
{
    int lpqp_idx = 0;
    int n;
    
    load_mb(p->mb, p->image, p->step, p->color.num_comp, mbx, mby, mb_window);
    //print_mb(p->mb[0], BW);print_mb(p->mb[1], BW);print_mb(p->mb[2], BW);
    fwd_color_conv(p->mb, mb_window, &p->color);
    //print_mb(p->mb[0], BW);print_mb(p->mb[1], BW);print_mb(p->mb[2], BW);
    
    for (n = 0; n < p->color.num_comp; n++) {
        int ch = n > 0 && p->color.is_yuv42x;
        fwd_trx_stage1(p->mb[n], p->mb_dclp[n], &mb_window[ch], p->param.overlap_mode);
        fwd_trx_stage2(p->mb_dclp[n], &mb_window[ch], p->param.overlap_mode);
        //TODO : Rate Control
        //TODO : Quantize
        //dclp[k][0] >>= 0;
    }
    //print_mb(p->mb[0], BW);//print_mb(p->mb[1], BW);
    //print_mb(p->mb[2], BW);
    //print_blk(p->mb_dclp[0], DCLPW, 4, 4);//print_blk(p->mb_dclp[1], DCLPW, 2, 2);print_blk(p->mb_dclp[2], DCLPW, 2, 2);    

    store_dclp(p->dclp_buf, p->mb_dclp, &p->color);
    store_param(&p->lpqp_idx_buf, lpqp_idx);
}


void enc_mb_stage2(jxr_t *p, rect_t *tile, rect_t *is_tile_edge, uint32_t mbx, uint32_t mby)
{
    int diff_cbphp[16];
    int lpqp_idx = 0;
    int hpqp_idx = 0;
    int n;
    int nbits, total = 0;
    
    load_pred_dclp(p->pred_dclp, p->dclp_buf, &p->color);
    load_hp(p->hp, p->mb, &p->color);
    fwd_pred_dclp(p->pred_dclp, p->dclp_buf, &p->lpqp_idx_buf, &p->color, is_tile_edge);
    //printf("\n");print_blk(p->pred_dclp[0], 4, 4, 4);print_blk(p->pred_dclp[1], 4, 4, 4);print_blk(p->pred_dclp[2], 4, 4, 4);
    //print_blk(p->hp[0][5], 4, 4, 4);
    p->mbhp_param.mode_hp = fwd_pred_hp(p->hp, p->mb_dclp, &p->color);
    //print_blk(p->hp[0][0], 4, 4, 4);
    //print_blk(p->mb[0], BW, 16, 16);
    
    if (is_tile_edge->left && is_tile_edge->top) {
        init_mbdc_param(&p->mbdc_param);
        init_mblp_param(&p->mblp_param);
        init_mbhp_param(&p->mbhp_param);
    }

    if ((mbx - tile->left) % 16 == 0) {
        reset_totals_adapt_scan_lp(&p->mblp_param.scan);
        reset_totals_adapt_scan_hp(p->mbhp_param.scan);
    }
    
    if (p->param.bands_present != DCONLY) {
        if (p->param.num_lpqps > 1) {
            enc_qp_index(p->out, p->param.num_lpqps, lpqp_idx);
        }
        if (p->param.bands_present != NOHIGHPASS
            && p->param.num_hpqps > 1) {
            enc_qp_index(p->out, p->param.num_hpqps, hpqp_idx); 
        }
    }
    
    //mark(p->out);
    //printf("{%d, %d, %d},\n", p->pred_dclp[0][0], p->pred_dclp[1][0], p->pred_dclp[2][0]);
    enc_mb_dc(p->out, &p->mbdc_param, &p->color, p->pred_dclp);
    //nbits=get_nbits(p->out);printf("|ENC DC   | %3d[bits]", nbits);total+=nbits;
     //printf("|ModelLP : %d, %d|\n",model[1].bits[0], model[1].bits[1]);
     //mark(p->out);
    if (p->param.bands_present != DCONLY) {
        enc_mb_lp(p->out, &p->mblp_param, &p->color, p->pred_dclp);
        //nbits=get_nbits(p->out);printf("|ENC LP   | %3d[bits]", nbits);total+=nbits;
        
        if (p->param.bands_present != NOHIGHPASS) {
            fwd_pred_cbphp(diff_cbphp, &p->mbhp_param, &p->color, is_tile_edge, p->cbphp_buf, p->hp);
            //mark(p->out);
            enc_mb_cbphp(p->out, &p->mbhp_param, &p->color, diff_cbphp);
            //nbits=get_nbits(p->out);printf("|ENC CBPHP| %3d[bits]", nbits);total+=nbits;
            //mark(p->out);
            enc_mb_hp_flex(p->out, &p->mbhp_param, &p->color, p->cbphp_buf, p->hp);
            //nbits=get_nbits(p->out);printf("|ENC HP   | %3d[bits]", nbits);total+=nbits;
            //printf("|Total  | %4d[bits]\n",total);
        }
    }

    if (is_tile_edge->right || (mbx - tile->left) % 16 == 0) {
        adapt_dc(p->mbdc_param.vlc);
        adapt_lp(p->mblp_param.vlc);
        adapt_hp(p->mbhp_param.vlc);
    }
    
    ringbuf_update(&p->lpqp_idx_buf);
    for (n = 0; n < p->color.num_comp; n++) {
        ringbuf_update(&p->dclp_buf[n]);
        ringbuf_update(&p->cbphp_buf[n]);
    }
    //printf("\n");
}


void dec_mb_stage1()
{
    
}


void dec_mb_stage2()
{
    
}
