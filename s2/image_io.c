#include <errno.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "s2.h"

int
image_load(const struct app_config *cfg, struct image *img)
{
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width;
	png_uint_32 height;
	png_byte color_type;
	png_byte bit_depth;
	png_bytep *rows;
	size_t rowbytes;
	size_t packed_rowbytes;
	size_t i;

	if (!cfg || !img) {
		fprintf(stderr, "s2: internal error: invalid load args\n");
		return -1;
	}

	memset(img, 0, sizeof(*img));

	fp = NULL;
	if (cfg->input_kind == INPUT_STDIN) {
		fp = stdin;
	} else if (cfg->input_kind == INPUT_FILE && cfg->input_path) {
		fp = fopen(cfg->input_path, "rb");
		if (!fp) {
			fprintf(stderr, "s2: cannot open input '%s': %s\n", cfg->input_path, strerror(errno));
			return -1;
		}
	}

	if (!fp) {
		fprintf(stderr, "s2: no input image\n");
		return -1;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		fprintf(stderr, "s2: failed to init png reader\n");
		if (fp != stdin) {
			fclose(fp);
		}
		return -1;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		fprintf(stderr, "s2: failed to init png info\n");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		if (fp != stdin) {
			fclose(fp);
		}
		return -1;
	}

	rows = NULL;
	if (setjmp(png_jmpbuf(png_ptr))) {
		fprintf(stderr, "s2: invalid or unsupported png input\n");
		if (rows) {
			free(rows);
		}
		image_free(img);
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		if (fp != stdin) {
			fclose(fp);
		}
		return -1;
	}

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);

	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	if (bit_depth == 16) {
		png_set_strip_16(png_ptr);
	}
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png_ptr);
	}
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		png_set_expand_gray_1_2_4_to_8(png_ptr);
	}
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(png_ptr);
	}
	if (color_type == PNG_COLOR_TYPE_RGB ||
	    color_type == PNG_COLOR_TYPE_GRAY ||
	    color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	}
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png_ptr);
	}

	png_read_update_info(png_ptr, info_ptr);
	rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	if (width == 0 || height == 0 || rowbytes == 0) {
		fprintf(stderr, "s2: invalid png dimensions\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		if (fp != stdin) {
			fclose(fp);
		}
		return -1;
	}

	packed_rowbytes = (size_t)width * 4;
	img->len = packed_rowbytes * (size_t)height;
	img->pixels = malloc(img->len);
	if (!img->pixels) {
		fprintf(stderr, "s2: out of memory while loading png\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		if (fp != stdin) {
			fclose(fp);
		}
		return -1;
	}

	rows = malloc(sizeof(*rows) * (size_t)height);
	if (!rows) {
		fprintf(stderr, "s2: out of memory while preparing png rows\n");
		image_free(img);
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		if (fp != stdin) {
			fclose(fp);
		}
		return -1;
	}

	for (i = 0; i < (size_t)height; i++) {
		rows[i] = img->pixels + (i * packed_rowbytes);
	}

	png_read_image(png_ptr, rows);
	png_read_end(png_ptr, NULL);

	img->width = (int)width;
	img->height = (int)height;

	free(rows);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	if (fp != stdin) {
		fclose(fp);
	}
	return 0;

}

int
image_write_png_stream(const struct image *img, void *stream)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep *rows;
	size_t i;
	size_t rowbytes;
	FILE *fp;

	if (!img || !img->pixels || img->width <= 0 || img->height <= 0 || !stream) {
		fprintf(stderr, "s2: invalid image for png write\n");
		return -1;
	}

	fp = stream;
	rowbytes = (size_t)img->width * 4;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		fprintf(stderr, "s2: failed to init png writer\n");
		return -1;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		fprintf(stderr, "s2: failed to init png info\n");
		png_destroy_write_struct(&png_ptr, NULL);
		return -1;
	}

	rows = NULL;
	if (setjmp(png_jmpbuf(png_ptr))) {
		fprintf(stderr, "s2: failed to write png\n");
		if (rows) {
			free(rows);
		}
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return -1;
	}

	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr,
	             info_ptr,
	             (png_uint_32)img->width,
	             (png_uint_32)img->height,
	             8,
	             PNG_COLOR_TYPE_RGBA,
	             PNG_INTERLACE_NONE,
	             PNG_COMPRESSION_TYPE_DEFAULT,
	             PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	rows = malloc(sizeof(*rows) * (size_t)img->height);
	if (!rows) {
		fprintf(stderr, "s2: out of memory while writing png\n");
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return -1;
	}

	for (i = 0; i < (size_t)img->height; i++) {
		rows[i] = (png_bytep)(img->pixels + (i * rowbytes));
	}

	png_write_image(png_ptr, rows);
	png_write_end(png_ptr, NULL);

	free(rows);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return 0;
}

int
image_save_png(const struct image *img, const char *path)
{
	FILE *fp;
	int rc;

	if (!path || !*path) {
		fprintf(stderr, "s2: invalid output path\n");
		return -1;
	}

	fp = fopen(path, "wb");
	if (!fp) {
		fprintf(stderr, "s2: cannot open output '%s': %s\n", path, strerror(errno));
		return -1;
	}

	rc = image_write_png_stream(img, fp);
	if (fclose(fp) != 0) {
		fprintf(stderr, "s2: failed to close output '%s'\n", path);
		return -1;
	}

	return rc;
}

void
image_free(struct image *img)
{
	if (!img) {
		return;
	}
	if (img->pixels) {
		free(img->pixels);
	}
	img->pixels = NULL;
	img->len = 0;
	img->width = 0;
	img->height = 0;
}
