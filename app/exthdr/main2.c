#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const uint8_t GDI_SIGNATURE[8] = {
	0x57, 0x4d, 0x50, 0x48,
	0x4f, 0x54, 0x4f, 0x00
};


int main(int argc, char *argv[])
{
    FILE *fp;
	uint8_t *buf;
    char fname[256];
	int fsize;
	char prefix[256];
	int i, j;

    if (argc != 2) {
        return 0;
    }

    fp = fopen(argv[1], "r");
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET); 	
	buf = (uint8_t*)malloc(fsize);
    fread(buf, 1, fsize, fp);
    fclose(fp);
	
	i = 0;
	while (argv[1][i] != '.') i++;
	for (j = 0; j < i; j++) prefix[j] = argv[1][j];
	sprintf(fname, "%s.jxs", prefix); 

	i = 0;
	j = 0;
	while (j < 8) {
		while (buf[i] != GDI_SIGNATURE[j]) i++;
		j++;
	}
	fp = fopen(fname, "w");
	fwrite(buf + i, 1, fsize - i + 1, fp);
	fclose(fp);
	   
 
    return 0;
}
