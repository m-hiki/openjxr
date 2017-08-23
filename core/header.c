#include <stdint.h>
#include "bitbuf.h"
#include "common.h"
#include "enc.h"
#include "dec.h"
#include "../openjxr.h"


void enc_image_header(bitbuf_t *b, param_t *p)
{
    int i;
    
    write_u(b, 0x574d5048, 32);// GDI_SIGNATURE
    write_u(b, 0x4f544f00, 32);
    write_u(b, 1, 4); // RESERVED_B
    write_u1(b, get_flag(p->flags, HARD_TILING_FLAG));
    write_u(b, 1, 3); // RESERVRD_C
    write_u1(b, get_flag(p->flags, TILING_FLAG));
    write_u1(b, get_flag(p->flags, FREQUENCY_MODE_CODESTREAM_FLAG));
    write_u(b, p->spatial_xfrm_subordinate, 3);
    write_u1(b, get_flag(p->flags, INDEX_TABLE_PRESENT_FLAG));
    write_u(b, p->overlap_mode, 2);
    write_u1(b, get_flag(p->flags, SHORT_HEADER_FLAG));
    write_u1(b, get_flag(p->flags, LONG_WORD_FLAG));
    write_u1(b, get_flag(p->flags, WINDOWING_FLAG));
    write_u1(b, get_flag(p->flags, TRIM_FLEXBITS_FLAG));
    write_u(b, 1, 3); // RESERVED_D
    write_u1(b, get_flag(p->flags, ALPHA_IMAGE_PLANE_FLAG));
    write_u(b, p->ex_color_format, 4);
    write_u(b, p->ex_bitdepth, 4);
    
    if (get_flag(p->flags, SHORT_HEADER_FLAG)) {
        write_u(b, p->width - 1, 16);
        write_u(b, p->height - 1, 16);
    } else {
        write_u(b, p->width - 1, 32);
        write_u(b, p->height - 1, 32);
    }

    if (get_flag(p->flags, TILING_FLAG)) {
        write_u(b, p->num_ver_tiles - 1, 12);
        write_u(b, p->num_hor_tiles - 1, 12);

        for (i = 0; i < p->num_ver_tiles - 1; i++) {
            if (get_flag(p->flags, SHORT_HEADER_FLAG)) {
                write_u(b, p->tile_width_in_mb[i], 8);
            } else {
                write_u(b, p->tile_width_in_mb[i], 16);
            }
        }
        
        for (i = 0; i < p->num_hor_tiles - 1; i++) {
            if (get_flag(p->flags, SHORT_HEADER_FLAG)) {
                write_u(b, p->tile_height_in_mb[i], 8);
            } else {
                write_u(b, p->tile_height_in_mb[i], 16);
            }
        }
    }
    
    if (get_flag(p->flags, WINDOWING_FLAG)) {
        write_u(b, p->margin.top, 6);
        write_u(b, p->margin.left, 6);
        write_u(b, p->margin.bottom, 6);
        write_u(b, p->margin.right, 6);
    }
}


static void enc_dc_qp(bitbuf_t *b, param_t *param)
{
    if (param->num_comp != 1) {
        write_u(b, param->component_mode, 2);
    }
        
    if (param->component_mode == UNIFORM) {
        write_u(b, param->dc_qp[0], 8);
    } else if (param->component_mode == SEPARATE) {
        write_u(b, param->dc_qp[0], 8);
        write_u(b, param->dc_qp[1], 8);
    } else if (param->component_mode == INDEPENDENT) {
        int i;
        
        for (i = 0; i < param->num_comp; i++) {
            write_u(b, param->dc_qp[i], 8);
        }
    }
}


static void enc_lp_qp(bitbuf_t *b, param_t *param)
{
    int i;

    for (i = 0; i < param->num_lpqps; i++) {
        if (param->num_comp != 1) {
            write_u(b, param->component_mode, 2);
        }
        
        if (param->component_mode == UNIFORM) {
            write_u(b, param->lp_qps[0][i], 8);
        } else if (param->component_mode == SEPARATE) {
            write_u(b, param->lp_qps[0][i], 8);
            write_u(b, param->lp_qps[1][i], 8);
        } else if (param->component_mode == INDEPENDENT) {
            int j;
            
            for (j = 0; j < param->num_comp; j++) {
                write_u(b, param->lp_qps[j][i], 8);
            }
        }
    }
}


static void enc_hp_qp(bitbuf_t *b, param_t *param)
{
    int i;
    
    for (i = 0; i < param->num_hpqps; i++) {
        if (param->num_comp != 1) {
            write_u(b, param->component_mode, 2);
        }
        
        if (param->component_mode == UNIFORM) {
            write_u(b, param->hp_qps[0][i], 8);
        } else if (param->component_mode == SEPARATE) {
            write_u(b, param->hp_qps[0][i], 8);
            write_u(b, param->hp_qps[1][i], 8);
        } else if (param->component_mode == INDEPENDENT) {
            int j;
            
            for (j = 0; j < param->num_comp; j++) {
                write_u(b, param->hp_qps[i][j], 8);
            }
        }
    }
}



void enc_image_plane_header(bitbuf_t *b, param_t *p)
{
    int flag;
    
    write_u(b, p->in_color_format, 3);
    write_u1(b, get_flag(p->flags, SCALING_FLAG));
    write_u(b, p->bands_present, 4);

    if (p->in_color_format == YUV444 ||
        p->in_color_format == YUV420 ||
        p->in_color_format == YUV422) {
        if (p->in_color_format == YUV420 ||
            p->in_color_format == YUV422) {
            write_u1(b, 0); // RESERVED_E_BIT
            write_u(b, p->chroma_centering_x, 3);
        } else {
            write_u(b, 0, 4); // RESERVED_F
        }
        
        if (p->in_color_format == YUV420) {
            write_u1(b, 0); // RESERVED_G_BIT
            write_u(b, p->chroma_centering_y, 3);
        } else {
            write_u(b, 0, 4); // RESERVED_H
        }
    } else if (p->in_color_format == NCOMPONENT) {
        write_u(b, p->num_comp - 1, 4);
        if (p->num_comp - 1 == 0xF) {
            //write_u(b, p->num_comps_extended_minus16, 12);
        }
    }
    
     if (p->ex_bitdepth == BD16 || p->ex_bitdepth == BD16S
         || p->ex_bitdepth == BD32S) {
         write_u(b, p->shift_bits, 8);
     } else if (p->ex_bitdepth == BD32F) {
         write_u(b, p->len_mantissa, 8);
         write_u(b, p->exp_bias, 8);
     }

    flag = get_flag(p->flags, DC_IMAGE_PLANE_UNIFORM_FLAG);
    write_u1(b, flag);
    
    if (flag) {
        enc_dc_qp(b, p);
        
        if (p->bands_present != DCONLY) {
            write_u1(b, 0); // RESERVED_I_BIT
            
            flag = get_flag(p->flags, LP_IMAGE_PLANE_UNIFORM_FLAG);
            write_u1(b, flag);

            if (flag) {
                enc_lp_qp(b, p);
            }
            
            if (p->bands_present != NOHIGHPASS) {
                write_u1(b, 0); // RESERVED_J_BIT
                flag = get_flag(p->flags, HP_IMAGE_PLANE_UNIFORM_FLAG);
                write_u1(b, flag);
                
                if (flag) {
                    enc_hp_qp(b, p);
                }
            }
        }
    }

    write_align(b);
}


static void enc_tile_header_dc(bitbuf_t *b, param_t *p) {
    int flag = get_flag(p->flags, DC_IMAGE_PLANE_UNIFORM_FLAG);

    if (!flag) {
        enc_dc_qp(b, p);
    }
}


static void enc_tile_header_lp(bitbuf_t *b, param_t *p) {
    int flag = get_flag(p->flags, LP_IMAGE_PLANE_UNIFORM_FLAG);
    
    if (!flag) {
        flag = get_flag(p->flags, USE_DC_QP_FLAG);
        write_u1(b, flag);
        
        if (!flag) {
            write_u(b, p->num_lpqps - 1, 4);
            enc_lp_qp(b, p);
        }
    }
}


static void enc_tile_header_hp(bitbuf_t *b, param_t *p) {
    int flag = get_flag(p->flags, HP_IMAGE_PLANE_UNIFORM_FLAG);

    if (!flag) {
        flag = get_flag(p->flags, USE_LP_QP_FLAG);
        write_u1(b, flag);
        
        if (!flag) {
            write_u(b, p->num_hpqps - 1, 4);
            enc_hp_qp(b, p);
        }
    }
}


void enc_tile_header(bitbuf_t *b, param_t *p)
{
    if (!get_flag(p->flags, FREQUENCY_MODE_CODESTREAM_FLAG)) {
        write_u(b, 1, 24); // TILE_STARTCODE u(24)
        write_u(b, 0, 8);
        if (get_flag(p->flags, TRIM_FLEXBITS_FLAG)) {
            write_u(b, p->trim_flexbits, 4);
        }
        enc_tile_header_dc(b, p);
        enc_tile_header_lp(b, p);
        enc_tile_header_hp(b, p);
    } else {
        int i;
        for (i = 0; i < 4; i++) {
            write_u(&b[i], 1, 24);
            write_u(&b[i], 0, 8);
        }
        
        enc_tile_header_dc(&b[0], p);
        if (p->bands_present != DCONLY) {
            enc_tile_header_lp(&b[1], p);
            if (p->bands_present != NOHIGHPASS) {
                enc_tile_header_hp(&b[2], p);
            }
        }
        
        if (get_flag(p->flags, TRIM_FLEXBITS_FLAG)) {
            write_u(&b[3], p->trim_flexbits, 4);
        }
    }
}


static const uint8_t BITS_QP_INDEX[17] = {
    0, 0, 1, 1,
    2, 2, 3, 3,
    3, 3, 4, 4,
    4, 4, 4, 4, 4
};


void enc_qp_index(bitbuf_t *b, int num_qp, uint8_t qp_index)
{
    int bits = BITS_QP_INDEX[num_qp];
    
    if (qp_index > 0) {
        write_u1(b, 1);
        write_u(b, qp_index - 1, bits);
    } else {
        write_u1(b, 0);
    }
}


int dec_qp_index(bitbuf_t *b, int num_qp)
{
    int bits = BITS_QP_INDEX[num_qp];
    int is_nonzero = read_u1(b);
    
    if (is_nonzero) {
        return read_u(b, bits) + 1;
    } else {
        return 0;
    }
}


void enc_index_table_tiles(bitbuf_t *b, int is_frequency_mode)
{
    int val;
    int i;
    
    if (is_frequency_mode) {
        //val = num_hor_tiles * num_ver_tiles * num_bands_of_primary;
    } else {
        //val = num_hor_tiles * num_ver_tiles;
    }
    
    write_u(b, 1, 16);
    
    for (i = 0; i < val; i++) {
        //VLW_ESC()
    }
}


int dec_image_header(bitbuf_t *b, param_t *p)
{
    int i;
    
	printf("hoge\n");
    if (read_u(b, 32) != 0x574d5048) return -1;
    if (read_u(b, 32) != 0x4f544f00) return -1;

    read_u(b, 4);// RESERVED_B
    set_flag(&p->flags, HARD_TILING_FLAG, read_u1(b));
    read_u(b, 3);// RESERVED_C
    set_flag(&p->flags, TILING_FLAG, read_u1(b));
    set_flag(&p->flags, FREQUENCY_MODE_CODESTREAM_FLAG, read_u1(b));
    /*
    write_u(b, 1, 4); // RESERVED_B
    write_u1(b, get_flag(p->flags, HARD_TILING_FLAG));
    write_u(b, 1, 3); // RESERVRD_C
    write_u1(b, get_flag(p->flags, TILING_FLAG));
    write_u1(b, get_flag(p->flags, FREQUENCY_MODE_CODESTREAM_FLAG));
    write_u(b, p->spatial_xfrm_subordinate, 3);
    write_u1(b, get_flag(p->flags, INDEX_TABLE_PRESENT_FLAG));
    write_u(b, p->overlap_mode, 2);
    write_u1(b, get_flag(p->flags, SHORT_HEADER_FLAG));
    write_u1(b, get_flag(p->flags, LONG_WORD_FLAG));
    write_u1(b, get_flag(p->flags, WINDOWING_FLAG));
    write_u1(b, get_flag(p->flags, TRIM_FLEXBITS_FLAG));
    write_u(b, 1, 3); // RESERVED_D
    write_u1(b, get_flag(p->flags, ALPHA_IMAGE_PLANE_FLAG));
    write_u(b, p->ex_color_format, 4);
    write_u(b, p->ex_bitdepth, 4);

     */    
    return 0;
}
