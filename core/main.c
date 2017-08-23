#include <stdio.h>
#include <stdint.h>
#include "../openjxr.h"


uint8_t *read_file_header(uint8_t *buf)
{
    printf("FIXED_FILE_HEADER_II_2BYTES | 0x%x\n", (buf[0] << 8) | buf[1] );
    printf("FIXED_FILE_HEADER_0XBC_BYTE | 0x%x\n", buf[2]);
    printf("FILE_VERSION_ID             | %d\n", buf[3]);
    printf("FIRST_IFD_OFFSET            | %d\n", (buf[7] << 24) |
           (buf[6] << 16) | (buf[5] << 8) | buf[4]);
    
    return buf + 8;
}


uint8_t *read_image_file_directory(uint8_t *buf)
{
    int num_entries = (buf[1] << 8) | buf[0];
    int i;
    
    printf("NUM_ENTRIES                 | %d\n", num_entries);
    buf += 2;
    
    for (i = 0; i < num_entries; i++) {
        int field_tag = (buf[1] << 8) | buf[0];
        int element_type = (buf[3] << 8) | buf[2];
        int num_elements = (buf[7] << 24) | (buf[6] << 16) | (buf[5] << 8) | buf[4];
        int values_or_offset = (buf[11] << 24) | (buf[10] << 16) | (buf[9] << 8) | buf[8];
        printf("FIELD_TAG                   | 0x%x\n", field_tag);
        printf("ELEMENT_TYPE                | %d\n", element_type);
        printf("NUM_ELEMENTS                | %d\n", num_elements);
        printf("VALUES_OR_OFFSET            | %d\n", values_or_offset);
        buf += 12;
    }

    printf("ZERO_OR_NEXT_IFD_OFFSET     | %d\n", (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
    buf += 4;
    printf("0x");
    for (i = 0; i < 16; i++) {
        printf("%x", *buf++);
    }
    printf("\n");
    //printf("%x", *buf++);
    //printf("%x", *buf++);
    //printf("%x", *buf++);
    //printf("%x", *buf++);
    //printf("\n");
    return buf;
}


int main(int argc, char *argv[])
{
    openjxr_t openjxr;
    FILE *fp;
    uint8_t buf[1024];
    uint8_t *p;
    
    if (argc != 2) {
        return 0;
    }

    fp = fopen(argv[1], "r");
    fread(buf, 1, 1024, fp);
    p = buf;

    p = read_file_header(p);
    p = read_image_file_directory(p);

    openjxr = openjxr_alloc(JXR_DECODER);
	printf("hoge0\n");    
    decode_header(openjxr, p);
    printf("hoge\n");
    
    openjxr_free(openjxr);

    fclose(fp);
    
    return 0;
}
