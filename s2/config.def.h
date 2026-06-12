/* See LICENSE file for copyright and license details. */

/* runtime behavior */
static const char *default_window_class = "s2";
static const int default_fill_mode = 0;
static const char *default_save_directory = "~/screenshots";
static const int window_padding = 16;

/* tool palette (hex RGB, no leading # in code) */
static const unsigned int palette[] = {
	0xff0000, /* red */
	0x00ff00, /* green */
	0x0000ff, /* blue */
	0xffff00, /* yellow */
	0xff8800, /* orange */
	0xffffff, /* white */
	0x000000, /* black */
	0x00ffff, /* cyan */
	0xff00ff, /* magenta */
};

/* default tool values */
static const int default_palette_index = 8;
static const int default_thickness_index = 4;
static const int default_text_scale = 4;
static const int default_highlight_strength = 25;
static const int default_pixelate_block = 8;
static const int default_blur_radius = 5;
static const int default_marker_height = 24;   /* text-highlight band height (image px) */
static const int default_marker_strength = 40;  /* text-highlight alpha/strength (1..100) */

/* UI defaults */
/* tool index mapping: 0=select, 1=arrow, 2=line, 3=pen, 4=number, 5=rect, 6=circle, 7=text, 8=highlight, 9=marker, 10=pixelate, 11=blur, 12=picker */
static const int default_tool_index = 1;
static const unsigned int selection_bbox_color = 0x00ffffu;

/* stroke/effect presets */
static const int thickness_presets[] = { 1, 2, 3, 5, 8 };

/* text rendering */
static const char *font_name = "Noto Sans, Noto Color Emoji:size=16";
static const int text_fill_padding = 1;
static const int text_fill_corner_radius = 2;
static const int max_text_scale = 16;
//static const char *font_name = "DejaVu Serif:size=16";
/* if you don't care about emojis you can use a cool monospace defaul
 * static const char *font_name = "monospace:size=16";
 */
