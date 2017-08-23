#include <stdint.h>
#include <stdint.h>
#include <stdlib.h>
#include "../openjxr.h"
#include "common.h"
#include "enc.h"
#include "dec.h"

void enc_alloc(jxr_t *p);
void enc_tile(jxr_t *p, rect_t *tile);
void enc_free(jxr_t *p);


openjxr_t openjxr_alloc(int mode)
{
    openjxr_t openjxr;

    openjxr = (openjxr_t)malloc(sizeof(jxr_t));
    
    if (mode == JXR_ENCODER) {
        ((jxr_t*)openjxr)->mode = mode;
        enable_hard_tiling(openjxr);
        disable_tiling(openjxr);
        disable_windowing(openjxr);
        disable_long_word(openjxr);
        disable_trim_flexbits(openjxr);
        disable_alpha_image_plane(openjxr);
        set_overlap_mode(openjxr, 1);
        set_internal_clr_fmt(openjxr, YUV444, 3);
        select_spatial_mode(openjxr);
        set_bands_present(openjxr, ALL);
        enable_dc_image_plane_uniform(openjxr);
        enable_lp_image_plane_uniform(openjxr);
        enable_hp_image_plane_uniform(openjxr);
    } else if (mode == JXR_DECODER) {
		((jxr_t*)openjxr)->mode = mode;
		printf("%p, %p\n", openjxr, (jxr_t*)openjxr);
    } else {
        return NULL;
    }

    return openjxr;
}


const unsigned char *encode_header(openjxr_t openjxr, int *len)
{
    jxr_t *p = (jxr_t*)openjxr;
    
    if (p->mode != JXR_ENCODER) return NULL;
    enc_alloc(p);
    
    p->is_ready = 1;
    
    return 0;
}


const unsigned char *encode(openjxr_t openjxr, const unsigned char *input_image, int *len)
{
    jxr_t *p = (jxr_t*)openjxr;
    rect_t tile;
    int i, mbx, mby;

    if (p == NULL) return NULL;
    if (p->mode != JXR_ENCODER) return NULL;
    if (!p->is_ready) return NULL;

    p->image = input_image;

    mbx = mby = 0;
    for (i = 0; i < p->param.num_ver_tiles * p->param.num_hor_tiles; i++) {
        tile.left = mbx;
        tile.right = mbx + p->param.tile_width_in_mb[i] - 1;
        tile.top = mby;
        tile.bottom = mby + p->param.tile_height_in_mb[i] - 1;
    
        //printf("%d, %d, %d, %d\n", tile.left, tile.right, tile.top, tile.bottom);
        enc_tile(p, &tile);
        
        mbx += p->param.tile_width_in_mb[i];
        mby += p->param.tile_height_in_mb[i];
    }

    if (get_flag(p->param.flags, FREQUENCY_MODE_CODESTREAM_FLAG)) {
        concat(&p->out[0], &p->out[1]);
        concat(&p->out[0], &p->out[2]);
        concat(&p->out[0], &p->out[3]);
    }

    p->out->p = p->out->buf;
    p->out->left = 32;

    *len = (int)(p->out[0].p - p->out[0].buf);
    return (const unsigned char*)p->out[0].buf;
}
#include <stdio.h>

int decode_header(openjxr_t openjxr, const unsigned char *input_bitstream)
{
    jxr_t *p = (jxr_t*)openjxr;

    if (p == NULL) return -1;
	printf("hoge1 %p \n", p);
    if (p->mode != JXR_DECODER) return -1;
    p->out = (bitbuf_t*)malloc(sizeof(bitbuf_t));
    
    p->out->p = (uint32_t*)input_bitstream;
    p->out->left = 32;
#ifndef __BIG_ENDIAN__
    *(p->out->p) = conv_endian(*(p->out->p));
#endif
    if (dec_image_header(p->out, &p->param)) return -1;

    p->is_ready = 1;
    return 0;
}


const unsigned char *decode(openjxr_t openjxr, const unsigned char *input_bitstream)
{
    jxr_t *p = (jxr_t*)openjxr;
    rect_t tile;
    int i, mbx, mby;
    
    if (p == NULL) return NULL;
    if (p->mode != JXR_DECODER) return NULL;
    if (!p->is_ready) return NULL;

	return 0; 
}


void openjxr_free(openjxr_t openjxr)
{
    jxr_t *p = (jxr_t*)openjxr;
    if (p->mode == JXR_ENCODER) {
        enc_free(p);
    }
}


int set_size(openjxr_t jxr, unsigned int width, unsigned int height)
{
    if (jxr) {
        if (width > 0 && height > 0) {
            jxr_t *p = (jxr_t*)jxr;
            p->param.width = width;
            p->param.height = height;
            //TODO : support ex width & height
            p->param.ex_width = width;
            p->param.ex_height = height;
            p->param.mb_width = p->param.ex_width >> 4;
            p->param.mb_height = p->param.ex_height >> 4;
            return 0;
        } else {
            return -2;
        }
    } else {
        return -1;
    }
}


unsigned int get_width(openjxr_t enc)
{
    if (enc) {
        return ((jxr_t*)enc)->param.width;
    } else {
        return -1;
    }
}


unsigned int get_height(openjxr_t enc)
{
    if (enc) {
        return ((jxr_t*)enc)->param.width;
    } else {
        return -1;
    }
}


int set_internal_clr_fmt(openjxr_t enc, int color_format, int num_component)
{
    if (enc) {
        jxr_t *p = (jxr_t*)enc;
        if ((color_format == YONLY && num_component == 1) 
            || (color_format == YUV420 && num_component == 3)
            || (color_format == YUV422 && num_component == 3)
            || (color_format == YUV444 && num_component == 3) 
            || (color_format == YUVK && num_component == 4)
            || (color_format == NCOMPONENT && num_component <= 16)) {
            p->param.in_color_format = color_format;
            switch (color_format) {
            case YUV444 :
                p->color.num_comp = 3;
                p->color.is_yuv420 = 0;
                p->color.is_yuv422 = 0;
                p->color.is_yuv444 = 1;
                p->color.is_yuv42x = 0;
                p->color.is_yuv4xx = 1;
                p->color.is_yuv = 1;
                p->color.is_yonly = 0;
                break;
            case YUV422 :
                p->color.num_comp = 3;
                p->color.is_yuv420 = 0;
                p->color.is_yuv422 = 1;
                p->color.is_yuv444 = 0;
                p->color.is_yuv42x = 1;
                p->color.is_yuv4xx = 1;
                p->color.is_yuv = 1;
                p->color.is_yonly = 0;
                break;
            case YUV420 :
                p->color.num_comp = 3;
                p->color.is_yuv420 = 1;
                p->color.is_yuv422 = 0;
                p->color.is_yuv444 = 0;
                p->color.is_yuv42x = 1;
                p->color.is_yuv4xx = 1;
                p->color.is_yuv = 1;
                p->color.is_yonly = 0;
                break;
            case YONLY :
                p->color.num_comp = 1;
                p->color.is_yuv420 = 0;
                p->color.is_yuv422 = 0;
                p->color.is_yuv444 = 0;
                p->color.is_yuv42x = 0;
                p->color.is_yuv4xx = 0;
                p->color.is_yuv = 0;
                p->color.is_yonly = 1;
                break;
            case YUVK :
                p->color.num_comp = 4;
                p->color.is_yuv420 = 0;
                p->color.is_yuv422 = 0;
                p->color.is_yuv444 = 0;
                p->color.is_yuv42x = 0;
                p->color.is_yuv4xx = 0;
                p->color.is_yuv = 1;
                p->color.is_yonly = 1;
                break;
            case NCOMPONENT :
                p->color.num_comp = num_component;
                p->color.is_yuv420 = 0;
                p->color.is_yuv422 = 0;
                p->color.is_yuv444 = 0;
                p->color.is_yuv42x = 0;
                p->color.is_yuv4xx = 0;
                p->color.is_yuv = 0;
                p->color.is_yonly = 0;
                break;
            }
            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}


int get_internal_clr_fmt(openjxr_t enc)
{
    if (enc) {
        return ((jxr_t*)enc)->param.in_color_format;
    } else {
        return -1;
    }
}


int get_internal_num_component(openjxr_t enc)
{
    if (enc) {
        return ((jxr_t*)enc)->color.num_comp;
    } else {
        return -1;
    }
}


int set_output_clr_fmt(openjxr_t enc, int color_format, int num_component, int bitdepth)
{
    if (enc) {
        jxr_t *p = (jxr_t*)enc;
        p->param.ex_color_format = color_format;
        p->param.num_comp = num_component;
        
        if (bitdepth == BD1WHITE1 || bitdepth == BD8
            || bitdepth == BD16 || bitdepth == BD16S
            || bitdepth == BD16F || bitdepth == BD32S
            || bitdepth == BD5 || bitdepth == BD565
            || bitdepth == BD1BLACK1) {
            p->param.ex_bitdepth = bitdepth;
            if (bitdepth == BD1WHITE1) {
                p->color.bias = 0;
            } else if (bitdepth == BD8) {
                p->color.bias = (1 << 7);
            } else if (bitdepth == BD16) {  
                p->color.bias = (1 << 15);
            } else if (bitdepth == BD5) {
                p->color.bias = (1 << 4);
            } else if (bitdepth == BD10) {
                p->color.bias = (1 << 9);
            } else if (bitdepth == BD565) {
                p->color.bias = (1 << 5);
            } else {
                p->color.bias = 0;
            }
        } else {
            return -2;
        }
        return 0;
    } else {
        return -1;
    }
}


int get_output_clr_fmt(openjxr_t enc)
{
    if (enc) {
        return ((jxr_t*)enc)->param.ex_color_format;
    } else {
        return -1;
    }
}


int get_output_num_component(openjxr_t enc)
{
    if (enc) {
        return ((jxr_t*)enc)->param.num_comp;
    } else {
        return -1;
    }
}


int get_output_bitdepth(openjxr_t enc)
{
    if (enc) {
        return ((jxr_t*)enc)->param.ex_bitdepth;
    } else {
        return -1;
    }
}


int select_spatial_mode(openjxr_t enc)
{
    if (enc) {
        set_flag(&((jxr_t*)enc)->param.flags, FREQUENCY_MODE_CODESTREAM_FLAG, 0);
        return 0;
    } else {
        return -1;
    }
}


int select_frequency_mode(openjxr_t enc)
{
    if (enc) {
        set_flag(&((jxr_t*)enc)->param.flags, FREQUENCY_MODE_CODESTREAM_FLAG, 1);
        return 0;
    } else {
        return -1;
    }
}


int is_spatial_mode(openjxr_t enc)
{
    if (enc) {
        return !get_flag(((jxr_t*)enc)->param.flags, FREQUENCY_MODE_CODESTREAM_FLAG);
    } else {
        return -1;
    }
}


int is_frequency_mode(openjxr_t enc)
{
    if (enc) {
        return get_flag(((jxr_t*)enc)->param.flags, FREQUENCY_MODE_CODESTREAM_FLAG);
    } else {
        return -1;
    }
}


int set_overlap_mode(openjxr_t enc, int level)
{
    if (enc) {
        if (level >= 0 && level <=2) {
            ((jxr_t*)enc)->param.overlap_mode = level;
            return 0;
        } else {
            return -2;
        }
    } else {
        return -1;
    }
}


int get_overlap_mode(openjxr_t enc)
{
    if (enc) {
        return ((jxr_t*)enc)->param.overlap_mode;
    } else {
        return -1;
    }
}


int set_bands_present(openjxr_t enc, int bands_present)
{
    if (enc) {
        if (bands_present >= 0 && bands_present <= 3) {
            jxr_t *p = (jxr_t*)enc;
            p->param.bands_present = bands_present;
            return 0;
        } else {
            return -2;
        }
    } else {
        return -1;
    }
}


int get_bands_present(openjxr_t enc)
{
    if (enc) {
        return ((jxr_t*)enc)->param.bands_present;
    } else {
        return -1;
    }
}


int set_spatial_xfrm_subordinate(openjxr_t enc, int spatial_xfrm_subordinate)
{
    if (enc) {
        jxr_t *p = (jxr_t*)enc;
        if (spatial_xfrm_subordinate >= 0 && spatial_xfrm_subordinate <= 7) {
            p->param.spatial_xfrm_subordinate = spatial_xfrm_subordinate;
            return 0;
        } else {
            return -2;
        }
    } else {
        return -1;
    }
}


int get_spatial_xfrm_subordinate(openjxr_t enc, int spatial_xfrm_subordinate)
{
    if (enc) {
        return ((jxr_t*)enc)->param.spatial_xfrm_subordinate;
    } else {
        return -1;
    }
}


#define DEFINE_FLAG(func, flag)                                 \
    int enable_##func(openjxr_t enc)                            \
    {                                                           \
        if (enc) {                                              \
            set_flag(&((jxr_t*)enc)->param.flags, flag, 1);     \
            return 0;                                           \
        } else {                                                \
            return -1;                                          \
        }                                                       \
    }                                                           \
    int disable_##func(openjxr_t enc)                           \
    {                                                           \
        if (enc) {                                              \
            set_flag(&((jxr_t*)enc)->param.flags, flag, 0);     \
            return 0;                                           \
        } else {                                                \
            return -1;                                          \
        }                                                       \
    }                                                           \
    int is_##func(openjxr_t enc)                                \
    {                                                           \
        if (enc) {                                              \
            return get_flag(((jxr_t*)enc)->param.flags, flag);  \
        } else {                                                \
            return -1;                                          \
        }                                                       \
    }

DEFINE_FLAG(tiling, TILING_FLAG)
DEFINE_FLAG(windowing, WINDOWING_FLAG)
DEFINE_FLAG(scaling, SCALING_FLAG)
DEFINE_FLAG(hard_tiling, HARD_TILING_FLAG)
DEFINE_FLAG(index_table_present, INDEX_TABLE_PRESENT_FLAG)
DEFINE_FLAG(alpha_image_plane, ALPHA_IMAGE_PLANE_FLAG)
DEFINE_FLAG(short_header, SHORT_HEADER_FLAG)
DEFINE_FLAG(long_word, LONG_WORD_FLAG)
DEFINE_FLAG(trim_flexbits, TRIM_FLEXBITS_FLAG)
DEFINE_FLAG(dc_image_plane_uniform, DC_IMAGE_PLANE_UNIFORM_FLAG)
DEFINE_FLAG(lp_image_plane_uniform, LP_IMAGE_PLANE_UNIFORM_FLAG)
DEFINE_FLAG(hp_image_plane_uniform, HP_IMAGE_PLANE_UNIFORM_FLAG)
