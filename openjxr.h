/*****************************************************************************
Copyright (c) 2010-2011 Minoru Hiki

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*******************************************************************************/
#ifndef OPENJXR_H_
#define OPENJXR_H_


/**
   OpenJXR codec object.
 */
typedef void* openjxr_t;


#define JXR_ENCODER  0
#define JXR_DECODER  1
#define MJX_ENCODER  2
#define MJX_DECODER  3


openjxr_t openjxr_alloc(int mode);
void openjxr_free(openjxr_t openjxr);


/**
   encode
 */
const unsigned char *encode_header(openjxr_t openjxr, int *len);
const unsigned char *encode(openjxr_t openjxr, const unsigned char *input_image, int *len);


/**
   decode
 */
int decode_header(openjxr_t openjxr, const unsigned char *input_bitstream);
const unsigned char *decode(openjxr_t openjxr, const unsigned char *input_bitstream);


/**
   
 */
int set_size(openjxr_t openjxr, unsigned int width, unsigned int height);


/**

 */
unsigned int get_width(openjxr_t openjxr);


/**
   
 */
unsigned int get_height(openjxr_t openjxr);

/**
   Defines of color format. 
   
 */
#define YONLY           0
#define YUV420          1
#define YUV422          2
#define YUV444          3
#define YUVK            4
#define CMYK            4
#define CMYKDIRECT      5
#define NCOMPONENT      6
#define RGB             7
#define RGBE            8

/**
   Defines of OUTPUT_BIT_DEPTH
 */
#define BD1WHITE1       0
#define BD8             1
#define BD16            2
#define BD16S           3
#define BD16F           4
#define BD32S           6
#define BD32F           7
#define BD5             8
#define BD10            9
#define BD565          10
#define BD1BLACK1      15

/**
   Functions of INTERNAL_COLOR_FORMAT
   
 */
int set_internal_clr_fmt(openjxr_t openjxr, int color_format, int num_component);


/**
   
 */
int get_internal_clr_fmt(openjxr_t openjxr);


/**
 */
int get_internal_num_component(openjxr_t openjxr);


/**
 */
int set_output_clr_fmt(openjxr_t openjxr, int color_format, int num_component, int bitdepth);


/**
   
 */
int get_output_clr_fmt(openjxr_t openjxr);


/**
 */
int get_output_num_component(openjxr_t openjxr);


/**
 */
int get_output_bitdepth(openjxr_t openjxr);


/**

 */
int select_spatial_mode(openjxr_t openjxr);


/**
 */
int select_frequency_mode(openjxr_t openjxr);


/**
 */
int is_spatial_mode(openjxr_t openjxr);


/**
 */
int is_frequency_mode(openjxr_t openjxr);


/**
 */
int set_overlap_mode(openjxr_t openjxr, int level);

/**
 */
int get_overlap_mode(openjxr_t openjxr);


//color component
//int set_dcqp(openjxr_t openjxr, int *qp);
//int set_lpqps(openjxr_t openjxr, int num, int **qps);
//int set_hpqps(openjxr_t openjxr, int num, int **qps);
//int set_tile_dcqp(openjxr_t openjxr, int n_tile, );
//int set_tile_lpqps();
//int set_tile_hpqps();

/**
 */
int enable_tiling(openjxr_t openjxr);


/**
 */
int disable_tiling(openjxr_t openjxr);


/**
 */
int is_tiling(openjxr_t openjxr);


/**
 */
int set_num_ver_tiles(openjxr_t openjxr, int num_ver_tiles);


/**
 */
int set_num_hor_tiles(openjxr_t openjxr, int num_hor_tiles);


/**
 */
int set_tile_width(openjxr_t openjxr, int tile_index, int tile_width_in_mb);


/**
 */
int set_tile_height(openjxr_t openjxr, int tile_index, int tile_height_in_mb);


/**
 */
int get_num_ver_tiles(openjxr_t openjxr);


/**
 */
int get_num_hor_tiles(openjxr_t openjxr);


/**
 */
int get_tile_width(openjxr_t openjxr, int tile_index);


/**
 */
int get_tile_height(openjxr_t openjxr, int tile_index);


/**
 */
int enable_windowing(openjxr_t openjxr);


/**
 */
int disable_windowing(openjxr_t openjxr);


/**
 */
int is_windowing(openjxr_t openjxr);


/**
 */
int set_margin(openjxr_t openjxr, int top, int left, int bottom, int right);


/**
 */
int get_margin_top(openjxr_t openjxr);


/**
 */
int get_margin_left(openjxr_t openjxr);


/**
 */
int get_margin_bottom(openjxr_t openjxr);


/**
 */
int get_margin_right(openjxr_t openjxr);


/**
 */
#define ALL             0
#define NOFLEXBITS      1
#define NOHIGHPASS      2
#define DCONLY          3

/**
 */
int set_bands_present(openjxr_t openjxr, int bands_present);


/**
 */
int get_bands_present(openjxr_t openjxr);


/**
 */
int enable_scaling(openjxr_t openjxr);


/**
 */
int disable_scaling(openjxr_t openjxr);


/**
 */
int is_scaling(openjxr_t openjxr);


/**
   Functions of HARD_TILING_FLAG
   
 */
int enable_hard_tiling(openjxr_t openjxr);


/**
 */
int disable_hard_tiling(openjxr_t openjxr);


/**
 */
int is_hard_tiling(openjxr_t openjxr);


/**
 */
int enable_index_table_present(openjxr_t openjxr);


/**
 */
int disable_index_table_present(openjxr_t openjxr);


/**
 */
int is_index_table_present(openjxr_t openjxr);


/**
   
 */
int enable_short_header(openjxr_t openjxr);
int disable_short_header(openjxr_t openjxr);
int is_short_header(openjxr_t openjxr);

int enable_long_word(openjxr_t openjxr);
int disable_long_word(openjxr_t openjxr);
int is_long_word(openjxr_t openjxr);

int enable_trim_flexbits(openjxr_t openjxr);
int disable_trim_flexbits(openjxr_t openjxr);
int is_trim_flexbits(openjxr_t openjxr);

int enable_alpha_image_plane(openjxr_t openjxr);
int disable_alpha_image_plane(openjxr_t openjxr);
int has_alpha_image_plane(openjxr_t openjxr);

int enable_dc_image_plane_uniform(openjxr_t openjxr);
int disable_dc_image_plane_uniform(openjxr_t openjxr);
int is_dc_image_plane_uniform(openjxr_t openjxr);

int enable_lp_image_plane_uniform(openjxr_t openjxr);
int disable_lp_image_plane_uniform(openjxr_t openjxr);
int is_lp_image_plane_uniform(openjxr_t openjxr);

int enable_hp_image_plane_uniform(openjxr_t openjxr);
int disable_hp_image_plane_uniform(openjxr_t openjxr);
int is_hp_image_plane_uniform(openjxr_t openjxr);


#define TL              0
#define BL              1
#define TR              2
#define BR              3
#define RT              4
#define RB              5
#define LT              6
#define LB              7

int set_spatial_xfrm_subordinate(openjxr_t openjxr, int spatial_xfrm_subordinate);
int get_spatial_xfrm_subordinate(openjxr_t openjxr, int spatial_xfrm_subordinate);


#endif /* OPENJXR_H_ */
