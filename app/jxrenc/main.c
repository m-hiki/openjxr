#include <stdint.h>
#include <stdio.h>
#include "../../openjxr.h"


static void analyze_args(openjxr_t openjxr, char *argv[])
{
    
    set_size(openjxr, width, height);
	set_internal_clr_fmt(openjxr, YUV444, 3);
    set_output_clr_fmt(openjxr, RGB, 3, BD8);
    openjxr_setup(openjxr);
}


static void print_help()
{
	printf("OpenJXR v1.0, \n");    
}


int main(int argc, char *argv[])
{
    uint8_t *img;
    openjxr_t openjxr;
    rect_t tile;
    int width = dim[TEST][0][0];
    int height = dim[TEST][0][1];
    int hp_mode;
    int i, j, k;
    int mbs = 0;
    int len;
    
    if (argc != 2) {
        return 0;
    }

    openjxr = openjxr_new(JXR_ENCODER);

    TIC;
    //for (i = 0; i < 300; i++) {
    encode(openjxr, img, &len);
    //}
    TOC;
    
	openjxr_free(openjxr);


    return 0;
}
