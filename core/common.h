#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>
#include "bitbuf.h"


#define MAX_COMPONENTS                 16
#define UNIFORM                        0
#define SEPARATE                       1
#define INDEPENDENT                    2
#define BAND_DC                        0
#define BAND_LP                        1
#define BAND_HP                        2
#define ALL                            0
#define NOFLEXBITS                     1
#define NOHIGHPASS                     2
#define DCONLY                         3
#define MAX_LPQPS                      16
#define MAX_HPQPS                      16
#define HARD_TILING_FLAG               1
#define TILING_FLAG                    1 << 1
#define FREQUENCY_MODE_CODESTREAM_FLAG 1 << 2
#define INDEX_TABLE_PRESENT_FLAG       1 << 3
#define SHORT_HEADER_FLAG              1 << 4
#define LONG_WORD_FLAG                 1 << 5
#define WINDOWING_FLAG                 1 << 6
#define TRIM_FLEXBITS_FLAG             1 << 7
#define ALPHA_IMAGE_PLANE_FLAG         1 << 8
#define SCALING_FLAG                   1 << 9
#define SCALING_FLAG_ALPHA             1 << 10
#define DC_IMAGE_PLANE_UNIFORM_FLAG    1 << 11
#define DC_IMAGE_PLANE_UNIFORM_FLAG_A  1 << 12
#define LP_IMAGE_PLANE_UNIFORM_FLAG    1 << 13
#define LP_IMAGE_PLANE_UNIFORM_FLAG_A  1 << 14
#define HP_IMAGE_PLANE_UNIFORM_FLAG    1 << 15
#define HP_IMAGE_PLANE_UNIFORM_FLAG_A  1 << 16
#define USE_DC_QP_FLAG                 1 << 17
#define USE_LP_QP_FLAG                 1 << 18


#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))



typedef struct {
    int conv_idx;
    int bias;
    int scale;
    int num_comp;
    int is_yuv420;
    int is_yuv422;
    int is_yuv444;
    int is_yuv42x;//YUV420 || YUV422
    int is_yuv4xx;//YUV420 || YUV422 || YUV444
    int is_yuv;   //YUV420 || YUV422 || YUV444 || YUVK
    int is_yonly;
} color_t;


typedef struct {
    int left;
    int right;
    int top;
    int bottom;
} rect_t;


typedef struct {
    int32_t *buf;
    int32_t *p;
    int size;
    int step;
} ringbuf_t;


typedef struct {
    int state[2];
    int bits[2];
} model_t;


typedef struct {
    int discrim_val1;
    int discrim_val2;
    int table_index;
    int delta_table_index;
    int delta2_table_index;
} adapt_vlc_t;


typedef struct {
    int count_max; // 1 | -8 : 7
    int count_zero;// 1 | -8 : 7
} cbplp_t;


typedef struct {
    int count_ones[2];
    int count_zeros[2];
    int state[2];
} cbphp_model_t;


typedef struct {
    int order[15];
    int totals[15];
} scan_t;


typedef struct {
    model_t model;
    adapt_vlc_t vlc[2];
} mbdc_param_t;


typedef struct {
    model_t model;
    adapt_vlc_t vlc[8];
    scan_t scan;
    cbplp_t cbplp;
} mblp_param_t;


typedef struct {
    model_t model;
    adapt_vlc_t vlc[10];
    scan_t scan[2];
    cbphp_model_t cbphp_model;
    int mode_hp;
    int trim_flexbits;
} mbhp_param_t;


typedef struct {
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    uint32_t ex_width;
    uint32_t ex_height;
    uint32_t mb_width;  // = ex_width / 16
    uint32_t mb_height; // = ex_height / 16
    uint32_t num_ver_tiles; // 1~4095
    uint32_t num_hor_tiles; // 1~4095
    uint16_t *tile_width_in_mb;
    uint16_t *tile_height_in_mb;
    rect_t margin; // Range : 0~63
    uint32_t overlap_mode; // 0~2
    int trim_flexbits; //4bits, 0~15
    int spatial_xfrm_subordinate;
    int ex_color_format;
    int ex_bitdepth;
    int num_comp;
    int in_color_format;
    int chroma_centering_x;
    int chroma_centering_y;
    int bands_present;
    int component_mode;
    int shift_bits;
    int len_mantissa;
    int exp_bias;
    uint32_t num_lpqps;
    uint32_t num_hpqps;
    uint8_t dc_qp[MAX_COMPONENTS];
    uint8_t lp_qps[MAX_COMPONENTS][MAX_LPQPS];
    uint8_t hp_qps[MAX_COMPONENTS][MAX_LPQPS];
} param_t;


typedef struct {
    int mode;
    int is_ready;
    param_t param;
    color_t color;
    mbdc_param_t mbdc_param;
    mblp_param_t mblp_param;
    mbhp_param_t mbhp_param;

    const uint8_t *image;
    int32_t step; // image step
    bitbuf_t *out;

    int **mb_buf;      // [num_comp][BW * BH], 2304nbytes
    int **mb;          // pointer to mb_buf
    int **mb_dclp_buf;
    int **mb_dclp;     //[num_comp][DCLPW * DCLPH], 192bytes
    int **pred_dclp;   //[num_comp][256/32/16], 
    int ***hp;         //[num_comp][16][256/32/16], 
    ringbuf_t *dclp_buf;
    ringbuf_t *cbphp_buf;
    ringbuf_t lpqp_idx_buf;
} jxr_t;


static inline void swap(int *x, int *y)
{
    int t = *x;
    *x = *y;
    *y = t;
}


static inline int clip(int x, int min, int max)
{
    if (x > max) x = max;
    if (x < min) x = min;
    return x;
}


static inline void set_flag(uint32_t *flags, uint32_t flag, int b)
{
    *flags = b ? (*flags | flag) : (*flags & ~flag);
}


static inline int get_flag(uint32_t flags, uint32_t flag)
{
    return (flags & flag) != 0;
}


void store_param(ringbuf_t *param_buf, int param);
int get_cur_param(ringbuf_t *param_buf);
int get_left_param(ringbuf_t *param_buf);
int get_top_param(ringbuf_t *param_buf);

void init_vlc_table1(adapt_vlc_t *vlc);
void init_vlc_table2(adapt_vlc_t *vlc);
void init_model_mb(model_t *model, int band);
int num_nonzero(int *coeff, int num, int model_bits);
int count_run_length(int runs[15], int levels[15], int coeff[15]);
void normalize_coeff(int *ncoeff, int *coeff, int num, int model_bits);
int fwd_adapt_scan(int *runs, int *levels, int *coeff, scan_t *scan, int model_bits);
//void fwd_adapt_scan(int *scan_coeff, int *norm_coeff, scan_t *scan);
void enc_refine(bitbuf_t *b, int coeff, int model_bits);
void adapt_vlc_table1(adapt_vlc_t *vlc);
void adapt_vlc_table2(adapt_vlc_t *vlc, int max_table_index);
void update_model_mb(int lap_mean[2], model_t *model, int band, color_t *color);
void reset_totals_adapt_scan_lp(scan_t *scan);
void enc_abs_level(bitbuf_t *b, adapt_vlc_t *vlc, uint32_t level);
void enc_block(bitbuf_t *b, adapt_vlc_t *vlc, int band, int is_chroma, 
               int runs[15], int levels[15], int num_nonzero, color_t *color);
int dec_abs_level(bitbuf_t *b, adapt_vlc_t *vlc);

extern const uint8_t SCAN_ORDER0[15];
extern const uint8_t SCAN_ORDER1[15];
extern const uint8_t SCAN_TOTALS[15];
extern const uint8_t TRANSPOSE444[16];
extern const uint8_t TRANSPOSE422[8];
extern const uint8_t TRANSPOSE420[4];


#endif /* COMMON_H_ */
