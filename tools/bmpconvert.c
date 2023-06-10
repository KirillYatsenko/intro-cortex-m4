/*
 * In order to understand how this program works, read:
 * https://en.wikipedia.org/wiki/BMP_file_format
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>

#define PIXEL_ARRAY_OFFSET		0x0A
#define WIDTH_OFFSET			0x12
#define HEIGHT_OFFSET			0x16
#define BYTES_PER_PIXEL			2
#define BYTES_PER_LINE			6

static void print_usage(void)
{
	printf("This program is used to convert bmp files into c array\n");
	printf("It also strips all unnecessary metadata\n\n");
	printf("Usage: ./bmpconvert <file_name>\n");
	printf("file_name: picture in bmp format with 16-bit color encoding\n");
	printf("Note: you can use stream redirection to output result to the file\n");
}

static int read_header_entry(FILE *file, long offset, void *data, uint8_t size)
{
	int ret;

	ret = fseek(file, offset, SEEK_SET);
	if (ret) {
		printf("Unable to call fseek on the file\n");
		return -1;
	}

	ret = fread(data, size, 1, file);
	if (ret != 1) {
		printf("Unable to read pixel array offset\n");
		return -1;
	}

	return 0;
}

static int exit_error(FILE *file)
{
	if (file)
		fclose(file);

	exit(-1);
}

static char *strtoup(char *src, char *dst)
{
	char *dst_start = dst;

	while (*src)
		*(dst++) = toupper(*(src++));

	*dst = '\0';

	return dst_start;
}

// Todo: print error message based on errno
int main(int argc, char *argv[])
{
	int ret;
	FILE *file;
	char *file_name, *file_ext;
	uint16_t pixel;
	uint32_t bitmap_offset, width, height, i;
	char file_name_up[1024];

	if (argc != 2 || !strncmp(argv[1], "--help", 6)) {
		print_usage();
		return -1;
	}

	file = fopen(argv[1], "r");
	if (!file) {
		printf("Unable to open provided file: '%s'\n", argv[1]);
		exit_error(NULL);
	}

	file_name = basename(argv[1]);
	file_ext = strstr(file_name, ".bmp");
	if (!file_ext) {
		printf("File format is not recognized, should be: .bmp\n");
		exit_error(file);
	}

	*file_ext = '\0'; // trim extension

	ret = read_header_entry(file, WIDTH_OFFSET, &width, sizeof(width));
	if (ret) {
		printf("Unable to read width size\n");
		exit_error(file);
	}

	ret = read_header_entry(file, HEIGHT_OFFSET, &height, sizeof(height));
	if (ret) {
		printf("Unable to read height size\n");
		exit_error(file);
	}

	ret = read_header_entry(file, PIXEL_ARRAY_OFFSET, &bitmap_offset,
							sizeof(bitmap_offset));
	if (ret) {
		printf("Unable to read pixel array offset\n");
		exit_error(file);
	}

	ret = fseek(file, bitmap_offset, SEEK_SET);
	if (ret) {
		printf("Unable to call fseek to bitmap data\n");
		exit_error(file);
	}

	printf("#ifndef _%s_H_\n", strtoup(file_name, file_name_up));
	printf("#define _%s_H_\n\n", file_name_up);
	printf("#include <stdint.h>\n\n");

	printf("const uint16_t %s_width = %d;\n", file_name, width);
	printf("const uint16_t %s_height = %d;\n\n", file_name, height);

	printf("const uint16_t %s[] = {", file_name);

	for (i = 0; i < width * height; i++) {
		// if first in the row
		if (i % BYTES_PER_LINE == 0)
			printf("\n\t");

		ret = fread(&pixel, BYTES_PER_PIXEL, 1, file);
		if (ret != 1) {
			printf("Unable to read pixel: #%d\n", i);
			exit_error(file);
		}

		printf("0x%.4x,", pixel);

		// if not last in the row
		if (i % BYTES_PER_LINE != BYTES_PER_LINE - 1)
			printf(" ");
	}

	printf("\n};\n\n");
	printf("#endif\n");

	fclose(file);

	return ret;
}
