#ifndef ENC_H_
#define ENC_H_


#define BW       24 // MB buffer width
#define BH       24 // MB buffer height
#define DCLPW     4 // DCLP buffer width
#define DCLPH     4 // DCLP buffer height
#define EX_S     -2 // Start index in non-edge (in pix)
#define EX_E     18 // End index in non-edge (in pix)
#define EX_EC    10 // End index in non-edge (in pix), chroma
#define NO_S      0 // Start index in edge (in pix)
#define NO_E     16 // End index in edge (in pix)
#define NO_EC     8 // End index in non-edge (in pix), chroma
#define EOBLK    12 // End of block index non-overlap
#define EOBLK_CH  6 // End of block index non-overlap chroma


void init_mbdc_param(mbdc_param_t *param);
void init_mblp_param(mblp_param_t *param);
void init_mbhp_param(mbhp_param_t *param);
void enc_image_header(bitbuf_t *b, param_t *p);
void enc_image_plane_header(bitbuf_t *b, param_t *p);
void enc_tile_header(bitbuf_t *b, param_t *p);
void enc_mb_stage1(jxr_t *p, rect_t *mb_window, rect_t *is_tile_edge, uint32_t mbx, uint32_t mby);
void enc_mb_stage2(jxr_t *p, rect_t *tile, rect_t *is_tile_edge, uint32_t mbx, uint32_t mby);
void fwd_color_conv(int **mb, rect_t *rect, color_t *color);
void fwd_trx_stage1(int *mb, int *dclp, rect_t *rect, uint32_t overlap_mode);
void fwd_trx_stage2(int *dclp, rect_t *rect, uint32_t overlap_mode);
void load_mb(int **mb, const uint8_t *image, uint32_t width, int num_comp, int mbx, int mby, rect_t *rect);
void store_dclp(ringbuf_t *dclp_buf, int **mb_dclp, color_t *color);
void store_param(ringbuf_t *param_buf, int param);
void load_pred_dclp(int **pred_dclp, ringbuf_t *dclp_buf, color_t *color);
void load_hp(int ***hp, int **mb, color_t *color);
void fwd_pred_dclp(int **pred_dclp, ringbuf_t *dclp_buf, ringbuf_t *lpqp_idx_buf, color_t *color, rect_t *is_edge);
int fwd_pred_hp(int ***hp, int **mb_dclp, color_t *color);
void fwd_pred_cbphp(int *diff_cbphp, mbhp_param_t *param, color_t *color, 
                    rect_t *is_edge, ringbuf_t *cbphp_buf, int ***hp);
void enc_qp_index(bitbuf_t *b, int num_qp, uint8_t qp_index);
void enc_mb_dc(bitbuf_t *b, mbdc_param_t *param, color_t *color, int **dclp);
void enc_mb_lp(bitbuf_t *b, mblp_param_t *param, color_t *color, int **dclp);
void enc_mb_cbphp(bitbuf_t *b, mbhp_param_t *param, color_t *color, int *diff_cbphp);
void enc_mb_hp_flex(bitbuf_t *b, mbhp_param_t *param, color_t *color, ringbuf_t *cbphp_buf, int ***hp);
void ringbuf_update(ringbuf_t *ringbuf);
int get_color_conv_idx(int ex_color_format, int ex_bitdepth, int in_color_format);
int get_bias(int ex_bitdepth);


static inline int normalize(int x, int model_bits)
{
   //int v = ABS(x) >> model_bits;
   //return x >= 0 ? v : -v;
   int s = x >> 31;
   return ((((x ^ s) - s) >> model_bits) ^ s) - s;
}


/*
 ------------------------------------------------
 CODED_IMAGE(){
     IMAGE_HEADER()
     IsCurrPlaneAlphaFlag = FLASE
     IMAGE_PLANE_HEADER()
     if (ALPHA_IMAGE_PLANE_FLAG) {
         IsCurrPlaneAlphaFlag = TRUE
         IMAGE_PLANE_HEADER()
     }
     if (INDEX_TABLE_PRESENT_FLAG) {
         INDEX_TABLE_TILES()
     }
     SubsequentBytes = VLW_ESC()
     if (SubsequentBytes > 0) {
         iBytes = PROFILE_LEVE_INFO()
         valueAdditionalBytes = SubsequentBytes = iBytes
         for (iBytes = 0; iBytes < valueAdditionalBytes; iByte++)
             RESEARVED_A_BYTE               u(8)
     }
     CODED_TILES()
 }
 */
/*
   Box Types: ‘mjxr’
   Container: Sample Table Box (‘stbl’)
   Mandatory: Yes
   Quantity:  Exactly one

   The format of a sample when the sample entry name is ‘mjxr’ is a CODED_IMAGE() as defined in ITU-T Rec. T.832
   | ISO/IEC 29122-2, without the IMAGE_HEADER(), the IMAGE_PLANE_HEADER( ) for the primary image plane,
   and (when applicable) the IMAGE_PLANE_HEADER( ) for the alpha image plane. Each image presented to a JPEG
   XR decoder is logically formed by appending the content of each sample to the content of the JPEG XR Header
   Box in its associated Visual Sample Entry.
   
   
   class MJXRSampleEntry() extends VisualSampleEntry (‘mjxr’) {
       JPEGXRInfoBox();
       JPEGXRHeaderBox();
       JPEGXRProfileBox(); // optional
       ColourInformationBox(); // optional
   }

   class JPEGXRInfoBox() extends FullBox('jxri', 0, version=0){
       UInt8[16]  PIXEL_FORMAT;
       UInt8      IMAGE_BAND_PRESENCE;
       UInt8      ALPHA_BAND_PRESENCE;
   }

   class JPEGXRHeaderBox() extends FullBox('jxrh', 0, version=0){
       IMAGE_HEADER();
       IsCurrPlaneAlphaFlag := FALSE;
       IMAGE_PLANE_HEADER();
       
       if (ALPHA_IMAGE_PLANE_FLAG) {
           IsCurrPlaneAlphaFlag := TRUE;
           IMAGE_PLANE_HEADER();
       }
   }
  
   class JPEGXRProfileBox() extends Box('jxrp'){
       PROFILE_LEVEL_INFO();
   }
 
   class ColourInformationBox extends Box(‘colr’){
       unsigned int(32) colour_type;

       if (colour_type == ‘nclx’)// on-screen colours
       {
           unsigned int(16) colour_primaries;
           unsigned int(16) transfer_characteristics;
           unsigned int(16) matrix_coefficients;
           unsigned int(1) full_range_flag;
           unsigned int(7) reserved = 0;
       }
       else if (colour_type == ‘rICC’)// <<ed:code TBD>>
       {
           ICC_profile; // restricted ICC profile
       }
       else if (colour_type == ‘prof’)
       {
           ICC_profile;// unrestricted ICC profile
       }
  }
  
 */

#endif /* ENC_H_ */
