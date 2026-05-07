#ifndef S2_H
#define S2_H

#include <stddef.h>

#ifndef VERSION
#define VERSION "dev"
#endif

enum input_kind {
	INPUT_NONE = 0,
	INPUT_FILE,
	INPUT_STDIN,
};

struct app_config {
	enum input_kind input_kind;
	const char *input_path;
	const char *output_path;
	const char *window_class;
	const char *save_dir_override;
	int copy_on_finish;
	int normal_window;
	int show_help;
	int show_version;
};

struct image {
	int width;
	int height;
	unsigned char *pixels;
	size_t len;
};

int parse_args(int argc, char *argv[], struct app_config *cfg);
void print_usage(const char *argv0);

int image_load(const struct app_config *cfg, struct image *img);
int image_save_png(const struct image *img, const char *path);
int image_write_png_stream(const struct image *img, void *stream);
void image_free(struct image *img);

int editor_run(const struct app_config *cfg, struct image *img);
int clipboard_copy_png(const struct image *img);
int clipboard_paste_text(char *out, size_t outlen);

void draw_arrow(struct image *img,
	       int x0,
	       int y0,
	       int x1,
	       int y1,
	       int thickness,
	       unsigned int color);
void draw_line(struct image *img,
	      int x0,
	      int y0,
	      int x1,
	      int y1,
	      int thickness,
	      unsigned int color);
void draw_circle(struct image *img,
		int x0,
		int y0,
		int x1,
		int y1,
		int thickness,
		unsigned int color);
void draw_circle_fill(struct image *img, int x0, int y0, int x1, int y1, unsigned int color);
void draw_rect(struct image *img, int x0, int y0, int x1, int y1, int thickness, unsigned int color);
void draw_rect_fill(struct image *img, int x0, int y0, int x1, int y1, unsigned int color);
void draw_highlight(struct image *img, int x0, int y0, int x1, int y1, unsigned int color, int strength);
void draw_text(struct image *img, int x, int y, const char *text, int scale, unsigned int color);
void draw_pixelate(struct image *img, int x0, int y0, int x1, int y1, int block);
void draw_blur(struct image *img, int x0, int y0, int x1, int y1, int radius);
unsigned int draw_sample_color(const struct image *img, int x, int y);

void die(const char *fmt, ...);

#endif
