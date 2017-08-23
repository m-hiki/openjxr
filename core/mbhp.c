#include <stdint.h>
#include "bitbuf.h"
#include "common.h"
#include "enc.h"


#define HOR 0
#define VER 1
#define ABS_LEVEL_IND_HP0    0// 4
#define ABS_LEVEL_IND_HP1    1// 5
#define FIRST_IND_HP_LUM     2// 8
#define FIRST_IND_HP_CHR     3// 9
#define IND_HP_LUM0          4//14
#define IND_HP_LUM1          5//15
#define IND_HP_CHR0          6//16
#define IND_HP_CHR1          7//17
#define NUM_CBPHP_VLC        vlc[8]//
#define NUM_BLKCBPHP_VLC     vlc[9]//



void init_mbhp_param(mbhp_param_t *param)
{
    int i;
    
    for (i = 0; i < 15; i++) {
        param->scan[HOR].order[i] = SCAN_ORDER0[i];
        param->scan[VER].order[i] = SCAN_ORDER1[i];
        param->scan[HOR].totals[i] = SCAN_TOTALS[i];
        param->scan[VER].totals[i] = SCAN_TOTALS[i];
    }
    
    init_model_mb(&param->model, BAND_HP);
    init_vlc_table1(&param->vlc[ABS_LEVEL_IND_HP0]);
    init_vlc_table1(&param->vlc[ABS_LEVEL_IND_HP1]);
    init_vlc_table2(&param->vlc[FIRST_IND_HP_LUM]);
    init_vlc_table2(&param->vlc[FIRST_IND_HP_CHR]);
    init_vlc_table2(&param->vlc[IND_HP_LUM0]);
    init_vlc_table2(&param->vlc[IND_HP_LUM1]);
    init_vlc_table2(&param->vlc[IND_HP_CHR0]);
    init_vlc_table2(&param->vlc[IND_HP_CHR1]);
    init_vlc_table1(&param->NUM_CBPHP_VLC);
    init_vlc_table1(&param->NUM_BLKCBPHP_VLC);

    param->cbphp_model.state[0] = 0;
    param->cbphp_model.state[1] = 0;
    param->cbphp_model.count_ones[0] = -4;
    param->cbphp_model.count_ones[1] = -4;
    param->cbphp_model.count_zeros[0] = 4;
    param->cbphp_model.count_zeros[1] = 4;
}


void adapt_hp(adapt_vlc_t *vlc)
{
    adapt_vlc_table1(&vlc[ABS_LEVEL_IND_HP0]);
    adapt_vlc_table1(&vlc[ABS_LEVEL_IND_HP1]);
    adapt_vlc_table2(&vlc[FIRST_IND_HP_LUM], 4);
    adapt_vlc_table2(&vlc[FIRST_IND_HP_CHR], 4);
    adapt_vlc_table2(&vlc[IND_HP_LUM0], 3);
    adapt_vlc_table2(&vlc[IND_HP_LUM1], 3);
    adapt_vlc_table2(&vlc[IND_HP_CHR0], 3);
    adapt_vlc_table2(&vlc[IND_HP_CHR1], 3);
    adapt_vlc_table1(&NUM_CBPHP_VLC);
    adapt_vlc_table1(&NUM_BLKCBPHP_VLC);
}


void reset_totals_adapt_scan_hp(scan_t *scan)
{
    int i;

    for (i = 0; i < 15; i++) {
        scan[HOR].totals[i] = SCAN_TOTALS[i];
        scan[VER].totals[i] = SCAN_TOTALS[i];
    }
}


static const uint8_t HIER_SCAN_ORDER[16] = {
    0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15};


static inline int num_ones(int x)
{
    x = (x & 0x5555) + (x >> 1 & 0x5555);
    x = (x & 0x3333) + (x >> 2 & 0x3333);
    x = (x & 0x0f0f) + (x >> 4 & 0x0f0f);
    return (x & 0x00ff) + (x >> 8 & 0x00ff);
}


static void calc_cbphp(int *cbphp, ringbuf_t *cbphp_buf, color_t *color,
                       model_t *model, int ***hp)
{
    int n, i;
        
    for (n = 0; n < color->num_comp; n++) {
        int is_chroma = n > 0;
        int num_blk = (is_chroma && color->is_yuv420) ? 4 
            : (is_chroma && color->is_yuv422 ? 8 : 16);
        
        cbphp[n] = 0;
        for (i = 0; i < num_blk; i++) {
            int k = num_blk == 16 ? HIER_SCAN_ORDER[i] : i;
            if (num_nonzero(hp[n][k], 16, model->bits[is_chroma])) {
                cbphp[n] |= 1 << i;
            }
        }
        store_param(&cbphp_buf[n], cbphp[n]);
    }
}


static void update_cbphp_model(cbphp_model_t *model, int is_chroma, int norig)
{
    int ndiff = 3;
    
    model->count_ones[is_chroma] += norig - ndiff;
    model->count_ones[is_chroma] = clip(model->count_ones[is_chroma], -16, 15);
    model->count_zeros[is_chroma] += 16 - norig - ndiff;
    model->count_zeros[is_chroma] = clip(model->count_zeros[is_chroma], -16, 15);

    if (model->count_ones[is_chroma] < 0) {
        if (model->count_ones[is_chroma] < model->count_zeros[is_chroma]) {
            model->state[is_chroma] = 1;
        } else {
            model->state[is_chroma] = 2;
        }
    } else if (model->count_zeros[is_chroma] < 0) {
        model->state[is_chroma] = 2;
    } else {
        model->state[is_chroma] = 0;
    }
}


static inline int fwd_pred_cbphp444(int cbp, cbphp_model_t *model, int is_chroma,
                                    rect_t *edge, ringbuf_t *cbphp_buf)
{
    int norig = num_ones(cbp);

    if (model->state[is_chroma] == 0) {
        cbp ^= (cbp & 0x3300) << 2;
        cbp ^= (cbp & 0x00cc) << 6;
        cbp ^= (cbp & 0x0033) << 2;
        cbp ^= 0x0020 & (cbp << 1);
        cbp ^= 0x0010 & (cbp << 3);
        cbp ^= 0x0002 & (cbp << 1);
            
        if (edge->left) {
            if (edge->top) {
                cbp ^= 1;
            } else {
                int top_cbphp = get_top_param(cbphp_buf);
                cbp ^= (top_cbphp >> 10) & 1;
            }
        } else {
            int left_cbphp = get_left_param(cbphp_buf);
            cbp ^= (left_cbphp >> 5) & 1;
        }
    } else if (model->state[is_chroma] == 2) {
        cbp ^= 0xffff;
    }
    
    update_cbphp_model(model, is_chroma, norig);

    return cbp;
}


static inline int fwd_pred_cbphp422(int cbp, cbphp_model_t *model,
                                    rect_t *edge, ringbuf_t *cbphp_buf)
{
    int norig = num_ones(cbp) * 2;
    
    if (model->state[1] == 0) {
        cbp ^= ((cbp & 0x30) << 2);
        cbp ^= ((cbp & 0x0c) << 2);
        cbp ^= ((cbp & 0x03) << 2);
        cbp ^= ((cbp & 0x01) << 1);
        if (edge->left) {
            if (edge->top) {
                cbp ^= 1;
            } else {
                int top_cbphp = get_top_param(cbphp_buf);
                cbp ^= (top_cbphp >> 6) & 1;
            }
        } else {
            int left_cbphp = get_left_param(cbphp_buf);
            cbp ^= (left_cbphp >> 1) & 1;
        }
    } else if (model->state[1] == 2) {
        cbp ^= 0x00ff;
    }
    update_cbphp_model(model, 1, norig);
    
    return cbp;
}


static inline int fwd_pred_cbphp420(int cbp, cbphp_model_t *model,
                                    rect_t *is_edge, ringbuf_t *cbphp_buf)
{
    int norig = num_ones(cbp) * 4;
    
    if (model->state[1] == 0) {
        cbp ^= (cbp & 0x03) << 2;
        cbp ^= 0x02 & (cbp << 1);
        
        if (is_edge->left) {
            if (is_edge->top) {
                cbp ^= 1;
            } else {
                int top_cbphp = get_top_param(cbphp_buf);
                cbp ^= (top_cbphp >> 2) & 1;
            }
        } else {
            int left_cbphp = get_left_param(cbphp_buf);
            cbp ^= (left_cbphp >> 1) & 1;
        }
    } else if (model->state[1] == 2) {
        cbp ^= 0x0f;
    }
    update_cbphp_model(model, 1, norig);
    
    return cbp;
}


void fwd_pred_cbphp(int *diff_cbphp, mbhp_param_t *param, color_t *color, 
                    rect_t *is_edge, ringbuf_t *cbphp_buf, int ***hp)
{
    model_t *model = &param->model;
    cbphp_model_t *cbphp_model = &param->cbphp_model;
    int num = color->is_yuv42x ? 1 : color->num_comp;
    int n;
    
    calc_cbphp(diff_cbphp, cbphp_buf, color, model, hp);

    for (n = 0; n < num; n++) {
        diff_cbphp[n] = fwd_pred_cbphp444(diff_cbphp[n], cbphp_model, n > 0, is_edge, &cbphp_buf[n]);
    }
    
    if (color->is_yuv422) {
        diff_cbphp[1] = fwd_pred_cbphp422(diff_cbphp[1], cbphp_model, is_edge, &cbphp_buf[1]);
        diff_cbphp[2] = fwd_pred_cbphp422(diff_cbphp[2], cbphp_model, is_edge, &cbphp_buf[2]);
    } else if (color->is_yuv420) {
        diff_cbphp[1] = fwd_pred_cbphp420(diff_cbphp[1], cbphp_model, is_edge, &cbphp_buf[1]);
        diff_cbphp[2] = fwd_pred_cbphp420(diff_cbphp[2], cbphp_model, is_edge, &cbphp_buf[2]);
    }
}


static const vlc_t REF_CBPHP1[15] = {
    {0, 0},
    {0x0, 2}, //      0001, num=1
    {0x1, 2}, //      0010, num=1
    {0x0, 2}, // 00,  0011, num=2
    {0x2, 2}, //      0100, num=1
    {0x1, 2}, // 01,  0101, num=2
    {0x4, 3}, // 100, 0110, num=2
    {0x0, 2}, //      0111, num=3
    {0x3, 2}, //      1000, num=1
    {0x5, 3}, // 101, 1001, num=2
    {0x6, 3}, // 110, 1010, num=2
    {0x1, 2}, //      1011, num=3
    {0x7, 3}, // 111, 1100, num=2
    {0x2, 2}, //      1101, num=3
    {0x3, 2}, //      1110, num=3
};


static inline void enc_refine_cbphp(bitbuf_t *b, int ref)
{
    if (ref > 0 && ref < 15) {
        write_e(b, REF_CBPHP1[ref]);
    }
}


static const int8_t NUM_CBPHP_DELTA[5] = {
    0, -1, 0, 1, 1
};


static const vlc_t NUM_CBPHP[2][5] = {
    {// Code 0
        {0x1, 1}, // 1
        {0x1, 2}, // 01
        {0x1, 3}, // 001
        {0x0, 4}, // 0000
        {0x1, 4}  // 0001
    },
    {// Code 1
        {0x1, 1}, // 1
        {0x0, 3}, // 000
        {0x1, 3}, // 001
        {0x2, 3}, // 010
        {0x3, 3}  // 011
    }
};


static const vlc_t NUM_BLKCBPHP1[2][5] = {//YONLY, NCOMPONENT, YUVK
    {// Code 0
        {0x1, 1}, // 1
        {0x1, 2}, // 01
        {0x1, 3}, // 001
        {0x0, 4}, // 0000
        {0x1, 4}  // 0000
    },
    {// Code 1
        {0x1, 1}, // 1
        {0x0, 3}, // 000
        {0x1, 3}, // 001
        {0x2, 3}, // 010
        {0x3, 3}  // 011
    }
};


static const vlc_t NUM_BLKCBPHP2[2][9] = {
    {// Code 0
        {0x2, 3}, // 010
        {0x0, 5}, // 0000 0
        {0x4, 4}, // 0010
        {0x1, 5}, // 0000 1
        {0x2, 5}, // 0001 0
        {0x1, 1}, // 1
        {0x3, 3}, // 011
        {0x3, 5}, // 0001 1
        {0x3, 4}  // 0011
    },
    {// Code 1
        {0x1, 1}, // 1
        {0x1, 3}, // 001
        {0x2, 3}, // 010
        {0x1, 4}, // 0001
        {0x1, 6}, // 0000 01
        {0x3, 3}, // 011
        {0x1, 5}, // 0000 1
        {0x0, 7}, // 0000 000
        {0x1, 7}  // 0000 001
    }
};


static const int8_t NUM_BLKCBPHP_DELTA1[5] = {
    0, -1, 0, 1, 1 //YONLY, NCOMPONENT, YUVK
};


static const int8_t NUM_BLKCBPHP_DELTA2[9] = {
    2, 2, 1, 1, -1, -2, -2, -2, -3
};


static const vlc_t CBPHP_VLC[3] = {//CHR_CBPHP, VAL_INC, CBPHP_CH_BLK
    {0x1, 1},
    {0x1, 2},
    {0x0, 2}
};


static const vlc_t NUM_CH_BLK[4] = {
    {0x1, 1},
    {0x1, 2},
    {0x0, 3},
    {0x1, 3}
};


static const uint8_t VAL[16] = {0, 1, 1, 2, 1, 3 ,3, 4, 1, 3, 3, 4, 2, 4, 4, 5};
static const uint8_t FLC[6] = {0, 2, 1, 2, 2, 0};
static const uint8_t OFF[6] = {0, 4, 2, 8, 12, 1};
static const uint8_t OUT[16] = {0, 15, 3, 12, 1, 2, 4, 8, 5, 6, 9, 10, 7, 11, 13, 14};


// tile spatial & frequency
void enc_mb_cbphp(bitbuf_t *b, mbhp_param_t *param, color_t *color, int *diff_cbphp)
{
    adapt_vlc_t *vlc = param->vlc;
    int num = color->is_yonly || color->is_yuv4xx ? 1 : color->num_comp;
    int n, i;

    for (n = 0; n < num; n++) {
        int ref_cbphp = 0;
        int ref_cbphp_u = 0;
        int ref_cbphp_v = 0;
        int num_cbphp = 0;
        
        for (i = 0; i < 4; i++) {
            int mask = 0x000f << (i << 2);//0, 4, 8, 12
            if (diff_cbphp[n] & mask) ref_cbphp |= 1 << i;

            if (color->is_yuv420) {
                if (diff_cbphp[1] & (1 << i)) ref_cbphp_u |= 1 << i;
                if (diff_cbphp[2] & (1 << i)) ref_cbphp_v |= 1 << i;
            } else if (color->is_yuv422) {
                int mask = 0x05 << ((i >> 1) << 2) << ((i & 1));
                if (diff_cbphp[1] & mask) ref_cbphp_u |= 1 << i;
                if (diff_cbphp[2] & mask) ref_cbphp_v |= 1 << i;
            } else if (color->is_yuv444) {
                int mask = 0x000f << (i << 2);
                if (diff_cbphp[1] & mask) ref_cbphp_u |= 1 << i;
                if (diff_cbphp[2] & mask) ref_cbphp_v |= 1 << i;
            }
        }
        
        ref_cbphp |= ref_cbphp_u | ref_cbphp_v;
        num_cbphp = num_ones(ref_cbphp);
        write_e(b, NUM_CBPHP[NUM_CBPHP_VLC.table_index][num_cbphp]);
        NUM_CBPHP_VLC.discrim_val1 += NUM_CBPHP_DELTA[num_cbphp];
        enc_refine_cbphp(b, ref_cbphp);
        
        for (i = 0; i < 4; i++) {
            if ((ref_cbphp >> i) & 1) {
                int num_blkcbp = 0;
                int blkcbp = 0;
                int chr_cbp = 0;
                int val = 0;
                int val_inc = 0;
                int code = 0;
                int code_len = 0;
                int code_inc = 0;

                blkcbp = (diff_cbphp[n] >> (i << 2)) & 0xf;
                if ((ref_cbphp_u >> i) & 1) chr_cbp |= 0x1;
                if ((ref_cbphp_v >> i) & 1) chr_cbp |= 0x2;
                val = num_ones(blkcbp);
                if (val >= 2 && blkcbp != 0x3 && blkcbp != 0xc) val++;
                code = OFF[val];
                code_len = FLC[val];
                while (OUT[code + code_inc] != blkcbp) code_inc++;
                num_blkcbp = val - 1;
                
                if (chr_cbp) {
                    num_blkcbp += 6;
                    if (num_blkcbp >= 8) {
                        val_inc = num_blkcbp - 8;
                        num_blkcbp = 8;
                    }
                }

                if (color->is_yuv4xx) {
                    write_e(b, NUM_BLKCBPHP2[NUM_BLKCBPHP_VLC.table_index][num_blkcbp]);
                    NUM_BLKCBPHP_VLC.discrim_val1 += NUM_BLKCBPHP_DELTA2[num_blkcbp];
                } else {
                    write_e(b, NUM_BLKCBPHP1[NUM_BLKCBPHP_VLC.table_index][num_blkcbp]);
                    NUM_BLKCBPHP_VLC.discrim_val1 += NUM_BLKCBPHP_DELTA1[num_blkcbp];
                }

                if (chr_cbp) {
                    write_e(b, CBPHP_VLC[chr_cbp - 1]);
                    if (num_blkcbp == 8) write_e(b, CBPHP_VLC[val_inc]);
                }
                
                if (code_len) write_u(b, code_inc, code_len);

                if (color->is_yuv444) {
                    if (chr_cbp & 1) {
                        int cbpchr = (diff_cbphp[1] >> (i << 2)) & 0xf;
                        write_e(b, NUM_CH_BLK[num_ones(cbpchr) - 1]);
                        enc_refine_cbphp(b, cbpchr);
                    }
                    
                    if (chr_cbp & 2) {
                        int cbpchr = (diff_cbphp[2] >> (i << 2)) & 0xf;
                        write_e(b, NUM_CH_BLK[num_ones(cbpchr) - 1]);
                        enc_refine_cbphp(b, cbpchr);
                    }
                } else if (color->is_yuv422) {
                    /*
                    iDiffCBPHP[0] |= ((iBlkCBPHP & 0x0F) << (iBlock * 4))
                    for (k = 0; k < 2; k++)
                        if ((iBlkCBPHP >> (k + 4)) & 0x01) {
                            iShift[4] = {0, 1, 4, 5}
                            CBPHP_CH_BLK
                            iCBPHPChr = iShift[CBPHP_CH_BLK + 1]
                            iDiffCBPHP[k + 1] |= (iCBPHPChr << iShift[iBlock])
                        }
                    */
                }
            }
        }
    }
}


static inline int enc_block_adaptive(bitbuf_t *b, adapt_vlc_t *vlc, color_t *color, 
                                     int is_chroma, scan_t *scan, int mode_hp, 
                                     int model_bits, int *block)
{
    int runs[16];
    int levels[16];
    int num_nonzero = 0;
    
    num_nonzero = fwd_adapt_scan(runs, levels, block, &scan[mode_hp == 1], model_bits);
    enc_block(b, vlc, BAND_HP, is_chroma, runs, levels, num_nonzero, color);

    return num_nonzero;
}


static inline void enc_block_flexbits(bitbuf_t *b, int model_bits, int trim_flexbits,
                                      int *block)
{
    int flexbits_left = model_bits - trim_flexbits;
    
    if (flexbits_left < 0) flexbits_left = 0;
    if (flexbits_left) {
        int i;
        
        for (i = 1; i < 16; i++) {
            enc_refine(b, block[TRANSPOSE444[i]] >> trim_flexbits, flexbits_left);
        }
    }
}


// spatial mode
void enc_mb_hp_flex(bitbuf_t *b, mbhp_param_t *param, color_t *color, 
                    ringbuf_t *cbphp_buf, int ***hp)
{
    model_t *model = &param->model;
    adapt_vlc_t *vlc = param->vlc;
    scan_t *scan = param->scan;
    int mode_hp = param->mode_hp;
    int trim_flexbits = param->trim_flexbits;
    int lap_mean[2] = {0, 0};
    int n, i;

    for (n = 0; n < color->num_comp; n++) {
        int is_chroma = n > 0;
        int num_blk = (is_chroma && color->is_yuv420) ? 4 
            : (is_chroma && color->is_yuv422 ? 8 : 16);
        int model_bits = model->bits[is_chroma];
        int cbphp = get_cur_param(&cbphp_buf[n]);
        
        for (i = 0; i < num_blk; i++) {
            int k = num_blk == 16 ? HIER_SCAN_ORDER[i] : i;

            if ((cbphp >> i) & 1) {
                int num = enc_block_adaptive(b, vlc, color, is_chroma, scan,
                                             mode_hp, model_bits, hp[n][k]);
                lap_mean[is_chroma] += num;
            }

            enc_block_flexbits(b, model_bits, trim_flexbits, hp[n][k]);
        }
    }
    update_model_mb(lap_mean, model, BAND_HP, color);
}


// frequency mode
void enc_mb_hp(bitbuf_t *b)
{

}

// frequency mode
void enc_mb_flexbits(bitbuf_t *b)
{

}


static void inv_adapt_hp_scan(int32_t *coeff, int component, int block, int i, int value)
{//AdaptiveHPScan(iComponent, iBlock, i, iValue) { 

    /* 
       if (MBHPMode = = 1) { // vertical scan order 
           k = HighpassVerScanOrder[i]  
           HighpassVerTotals[i]++  
           HPInputVLC[iComponent][iBlock][k] = iValue  
           if ((i > 1) && (HighpassVerTotals[i] > HighpassVerTotals[i−1])) {  
               valueTemp = HighpassVerTotals[i]  
               HighpassVerTotals[i] = HighpassVerTotals[i−1]  
               HighpassVerTotals[i−1] = valueTemp  
               valueTemp = HighpassVerScanOrder[i]  
               HighpassVerScanOrder[i] = HighpassVerScanOrder[i−1]  
               HighpassVerScanOrder[i−1] = valueTemp  
           }
       } else { // horizontal scan order
           k = HighpassHorScanOrder[i]  
           HighpassHorTotals[i]++  
           HPInputVLC[iComponent][iBlock][k] = iValue  
           if ((i > 1) &&  (HighpassHorTotals[i] > HighpassHorTotals[i−1])) {  
               valueTemp = HighpassHorTotals[i]  
               HighpassHorTotals[i] = HighpassHorTotals[i−1]  
               HighpassHorTotals[i−1] = valueTemp  
               valueTemp = HighpassHorScanOrder[i]  
               HighpassHorScanOrder[i] = HighpassHorScanOrder[i−1]  
               HighpassHorScanOrder[i−1] = valueTemp  
           }
       }  
*/
}

