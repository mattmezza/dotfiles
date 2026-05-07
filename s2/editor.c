#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <pango/pangocairo.h>
#include <ctype.h>
#include <locale.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include "s2.h"

#include "config.h"

#ifndef default_tool_index
#define default_tool_index 1
#endif

#ifndef selection_bbox_color
#define selection_bbox_color 0x00ffffu
#endif

#ifndef default_pixelate_block
#define default_pixelate_block 8
#endif

#ifndef default_blur_radius
#define default_blur_radius 2
#endif

#ifndef text_fill_padding
#define text_fill_padding 1
#endif

#ifndef text_fill_corner_radius
#define text_fill_corner_radius 2
#endif

#ifndef window_padding
#define window_padding 16
#endif

#ifndef max_text_scale
#define max_text_scale 16
#endif

enum tool {
	TOOL_SELECT = 0,
	TOOL_ARROW,
	TOOL_LINE,
	TOOL_PEN,
	TOOL_NUMBER,
	TOOL_RECT,
	TOOL_CIRCLE,
	TOOL_TEXT,
	TOOL_HIGHLIGHT,
	TOOL_PIXELATE,
	TOOL_BLUR,
	TOOL_PICKER,
};

enum action_type {
	ACTION_ARROW = 0,
	ACTION_LINE,
	ACTION_PEN,
	ACTION_NUMBER,
	ACTION_RECT,
	ACTION_CIRCLE,
	ACTION_TEXT,
	ACTION_HIGHLIGHT,
	ACTION_PIXELATE,
	ACTION_BLUR,
};

struct action {
	enum action_type type;
	int x0;
	int y0;
	int x1;
	int y1;
	int p0;
	unsigned int color;
	char *text;
	int *pen_points;
	int pen_len;
};

struct action_stack {
	struct action *items;
	size_t len;
	size_t cursor;
	size_t cap;
};

struct editor_state {
	Display *dpy;
	int screen;
	Window win;
	Pixmap backbuf;
	Cursor cursor;
	GC gc;
	XImage *ximg;
	int status_h;
	int status_pad;
	int win_w;
	int win_h;
	float scale;
	int canvas_x;
	int canvas_y;
	int canvas_w;
	int canvas_h;
	int view_x;
	int view_y;
	Colormap cmap;
	unsigned long window_bg;
	unsigned long status_bg;
	unsigned long status_fg;
	XIM xim;
	XIC xic;
	XftDraw *xftdraw;
	XftFont *xftfont_status;
	XftFont *xftfont_tool[9];
	struct image *img;
	struct image base;
	struct image rendered;
	struct image preview;
	struct action_stack actions;
	int dirty;
	int raster_dirty;
	int running;
	int should_save;
	int save_timestamp;
	int should_copy;
	int cancelled;
	enum tool tool;
	unsigned int color;
	int fill_mode;
	int thickness_idx;
	int text_scale;
	int pixelate_block;
	int blur_radius;
	int highlight_strength;
	int cursor_x;
	int cursor_y;
	int anchor_active;
	int anchor_x;
	int anchor_y;
	int text_mode;
	char text_buf[256];
	int text_len;
	int color_mode;
	char color_buf[7];
	int color_len;
	int mouse_b1_down;
	int mouse_anchor_set_on_press;
	int mouse_drag_moved;
	int selected_idx;
	int drag_active;
	int drag_origin_x;
	int drag_origin_y;
	int drag_dx;
	int drag_dy;
	int pen_active;
	int pen_color;
	int pen_thickness;
	int *pen_points;
	int pen_len;
	int pen_cap;
	int text_x;
	int text_y;
	int toolbar_hover;
	int show_help;
	int number_next;
	int suppress_escape_once;
	const struct app_config *cfg;
};

#define TEXT_RENDER_PAD 4
#define TEXT_SCALE_PX 8

static int push_action(struct action_stack *st, const struct action *in);
static int text_bg_padding_for_scale(int scale);
static int text_bg_radius_for_scale(int scale);
static void text_box_metrics(const struct editor_state *ed,
			     int x,
			     int y,
			     const char *text,
			     int scale,
			     int with_fill,
			     int *bx,
			     int *by,
			     int *bw,
			     int *bh);
static void set_tool(struct editor_state *ed, enum tool tool);

static void
build_font_pattern(char *out, size_t outlen, int px)
{
	char base[160];
	const char *src;
	char *size_pos;
	char *end;

	if (!out || outlen == 0) {
		return;
	}
	src = (font_name && *font_name) ? font_name : "monospace:size=16";
	snprintf(base, sizeof(base), "%s", src);
	size_pos = strstr(base, ":size=");
	if (size_pos) {
		*size_pos = '\0';
	}
	end = base + strlen(base);
	while (end > base && (end[-1] == ' ' || end[-1] == '\t')) {
		end--;
		*end = '\0';
	}
	if (base[0] == '\0') {
		snprintf(base, sizeof(base), "monospace");
	}
	snprintf(out, outlen, "%s:size=%d", base, px);
}

static void
build_font_pattern_primary(char *out, size_t outlen, int px)
{
	char tmp[192];
	char *comma;
	char *end;

	if (!out || outlen == 0) {
		return;
	}
	build_font_pattern(tmp, sizeof(tmp), px);
	comma = strchr(tmp, ',');
	if (comma) {
		*comma = '\0';
		end = comma;
		while (end > tmp && (end[-1] == ' ' || end[-1] == '\t')) {
			end--;
			*end = '\0';
		}
	}
	snprintf(out, outlen, "%s", tmp);
}

static void
build_font_css_family(char *out, size_t outlen)
{
	char base[192];
	char *size_pos;
	char *p;

	if (!out || outlen == 0) {
		return;
	}
	out[0] = '\0';
	snprintf(base, sizeof(base), "%s", (font_name && *font_name) ? font_name : "sans");
	size_pos = strstr(base, ":size=");
	if (size_pos) {
		*size_pos = '\0';
	}
	for (p = base; *p != '\0'; p++) {
		if (*p == ':') {
			*p = ' ';
		}
	}
	snprintf(out, outlen, "%s", base);
}

static int
render_text_pango(const struct editor_state *ed,
		      struct image *img,
		      int x,
		      int y,
		      const char *text,
		      int scale,
		      unsigned int color,
		      int *ink_x,
		      int *ink_y,
		      int *ink_w,
		      int *ink_h,
		      int measure_only,
		      int *out_w,
		      int *out_h)
{
	(void)ed;
	cairo_surface_t *surface;
	cairo_t *cr;
	PangoLayout *layout;
	PangoFontDescription *desc;
	PangoRectangle ink_rect;
	PangoRectangle logical_rect;
	char family[192];
	int w_px;
	int h_px;
	int draw_w;
	int draw_h;
	int draw_off_x;
	int draw_off_y;
	int ix0;
	int iy0;
	int ix1;
	int iy1;
	double sr;
	double sg;
	double sb;

	if (!text) {
		text = "";
	}
	if (scale < 1) {
		scale = 1;
	}
	w_px = 0;
	h_px = 0;

	if (!measure_only && (!img || !img->pixels || !ed)) {
		return -1;
	}

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		return -1;
	}
	cr = cairo_create(surface);
	if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return -1;
	}

	layout = pango_cairo_create_layout(cr);
	if (!layout) {
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return -1;
	}
	pango_layout_set_text(layout, text, -1);
	build_font_css_family(family, sizeof(family));
	desc = pango_font_description_from_string(family);
	if (!desc) {
		g_object_unref(layout);
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return -1;
	}
	pango_font_description_set_absolute_size(desc, (double)(TEXT_SCALE_PX * scale) * PANGO_SCALE);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
	pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);
	w_px = logical_rect.width;
	h_px = logical_rect.height;
	if (w_px <= 0) {
		w_px = ink_rect.width;
	}
	if (h_px <= 0) {
		h_px = ink_rect.height;
	}
	if (w_px < 0) {
		w_px = 0;
	}
	if (h_px < 0) {
		h_px = 0;
	}

	if (ink_x) {
		*ink_x = x + TEXT_RENDER_PAD;
	}
	if (ink_y) {
		*ink_y = y + TEXT_RENDER_PAD;
	}
	if (ink_w) {
		*ink_w = ink_rect.width > 0 ? ink_rect.width : w_px;
	}
	if (ink_h) {
		*ink_h = ink_rect.height > 0 ? ink_rect.height : h_px;
	}
	if (out_w) {
		*out_w = w_px;
	}
	if (out_h) {
		*out_h = h_px;
	}
	g_object_unref(layout);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	if (measure_only || w_px <= 0 || h_px <= 0) {
		return 0;
	}

	draw_w = (ink_rect.width > 0 ? ink_rect.width : w_px) + TEXT_RENDER_PAD * 2;
	draw_h = (ink_rect.height > 0 ? ink_rect.height : h_px) + TEXT_RENDER_PAD * 2;
	draw_off_x = TEXT_RENDER_PAD - ink_rect.x;
	draw_off_y = TEXT_RENDER_PAD - ink_rect.y;
	if (draw_w <= 0 || draw_h <= 0) {
		return 0;
	}

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, draw_w, draw_h);
	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		return -1;
	}
	cr = cairo_create(surface);
	if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return -1;
	}
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
	cairo_paint(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	layout = pango_cairo_create_layout(cr);
	if (!layout) {
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return -1;
	}
	pango_layout_set_text(layout, text, -1);
	build_font_css_family(family, sizeof(family));
	desc = pango_font_description_from_string(family);
	if (!desc) {
		g_object_unref(layout);
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return -1;
	}
	pango_font_description_set_absolute_size(desc, (double)(TEXT_SCALE_PX * scale) * PANGO_SCALE);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

	ix0 = draw_w;
	iy0 = draw_h;
	ix1 = -1;
	iy1 = -1;

	if (w_px > 0 && h_px > 0) {
		int ix;
		int iy;
		unsigned char *src;
		int stride;

		sr = (double)((color >> 16) & 0xff) / 255.0;
		sg = (double)((color >> 8) & 0xff) / 255.0;
		sb = (double)(color & 0xff) / 255.0;
		cairo_set_source_rgb(cr, sr, sg, sb);
		cairo_move_to(cr, (double)draw_off_x, (double)draw_off_y);
		pango_cairo_show_layout(cr, layout);
		cairo_surface_flush(surface);
		src = cairo_image_surface_get_data(surface);
		stride = cairo_image_surface_get_stride(surface);
		for (iy = 0; iy < draw_h; iy++) {
			for (ix = 0; ix < draw_w; ix++) {
				int tx = x + ix;
				int ty = y + iy;
				unsigned char *sp = src + (size_t)iy * (size_t)stride + (size_t)ix * 4u;
				unsigned char b = sp[0];
				unsigned char g = sp[1];
				unsigned char r = sp[2];
				unsigned char a = sp[3];
				if (a > 8 && tx >= 0 && ty >= 0 && tx < img->width && ty < img->height) {
					unsigned char *dst = img->pixels + ((size_t)ty * (size_t)img->width + (size_t)tx) * 4u;
					if (ix < ix0) ix0 = ix;
					if (iy < iy0) iy0 = iy;
					if (ix > ix1) ix1 = ix;
					if (iy > iy1) iy1 = iy;
					dst[0] = (unsigned char)(((unsigned int)dst[0] * (255u - a) + (unsigned int)r * a) / 255u);
					dst[1] = (unsigned char)(((unsigned int)dst[1] * (255u - a) + (unsigned int)g * a) / 255u);
					dst[2] = (unsigned char)(((unsigned int)dst[2] * (255u - a) + (unsigned int)b * a) / 255u);
					dst[3] = 0xff;
				}
			}
		}
	}

	if (ix1 >= ix0 && iy1 >= iy0) {
		if (ink_x) {
			*ink_x = x + ix0;
		}
		if (ink_y) {
			*ink_y = y + iy0;
		}
		if (ink_w) {
			*ink_w = ix1 - ix0 + 1;
		}
		if (ink_h) {
			*ink_h = iy1 - iy0 + 1;
		}
	}

	g_object_unref(layout);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	return 0;
}

static void
text_metrics_xft(const struct editor_state *ed, const char *text, int scale, int *tw, int *th)
{
	if (render_text_pango(ed, NULL, 0, 0, text, scale, 0, NULL, NULL, NULL, NULL, 1, tw, th) != 0) {
		int len;
		if (!text) {
			text = "";
		}
		if (scale < 1) {
			scale = 1;
		}
		len = (int)strlen(text);
		if (tw) {
			*tw = len * 6 * scale;
		}
		if (th) {
			*th = 8 * scale;
		}
	}
}

static int
number_radius_for_scale(int scale)
{
	if (scale < 1) {
		scale = 1;
	}
	return scale * 6 + 6;
}

static int
contains_ci(const char *s, const char *needle)
{
	size_t i;
	size_t n;

	if (!s || !needle) {
		return 0;
	}
	n = strlen(needle);
	if (n == 0) {
		return 1;
	}
	for (i = 0; s[i] != '\0'; i++) {
		size_t j;
		for (j = 0; j < n; j++) {
			if (s[i + j] == '\0') {
				break;
			}
			if (tolower((unsigned char)s[i + j]) != tolower((unsigned char)needle[j])) {
				break;
			}
		}
		if (j == n) {
			return 1;
		}
	}
	return 0;
}

static int
parse_hex_color_value(const char *s, unsigned int *out)
{
	unsigned int v;
	if (!s || !out || strlen(s) != 6) {
		return -1;
	}
	if (sscanf(s, "%x", &v) != 1) {
		return -1;
	}
	if (v > 0xffffffu) {
		return -1;
	}
	*out = v;
	return 0;
}

static int
color_is_dark(unsigned int rgb)
{
	unsigned int r = (rgb >> 16) & 0xffu;
	unsigned int g = (rgb >> 8) & 0xffu;
	unsigned int b = rgb & 0xffu;
	unsigned int y = (r * 299u + g * 587u + b * 114u) / 1000u;
	return y < 128u;
}

static unsigned int
inverse_color(unsigned int color)
{
	unsigned int r = (color >> 16) & 0xffu;
	unsigned int g = (color >> 8) & 0xffu;
	unsigned int b = color & 0xffu;
	unsigned int y = (r * 299u + g * 587u + b * 114u) / 1000u;
	return (y < 128u) ? 0xffffffu : 0x000000u;
}

static int
detect_dark_preference(Display *dpy)
{
	const char *theme;
	const char *cfg;
	char *xr;
	char *copy;
	char *line;

	theme = getenv("GTK_THEME");
	if (theme && contains_ci(theme, "dark")) {
		return 1;
	}
	theme = getenv("KDE_COLOR_SCHEME");
	if (theme && contains_ci(theme, "dark")) {
		return 1;
	}

	cfg = getenv("COLORFGBG");
	if (cfg && strchr(cfg, ';')) {
		const char *p = strrchr(cfg, ';');
		if (p && *(p + 1)) {
			int bg = atoi(p + 1);
			if (bg >= 0 && bg <= 6) {
				return 1;
			}
			if (bg == 7 || bg == 15) {
				return 0;
			}
		}
	}

	xr = XResourceManagerString(dpy);
	if (!xr) {
		return 0;
	}
	copy = strdup(xr);
	if (!copy) {
		return 0;
	}
	line = strtok(copy, "\n");
	while (line) {
		char *colon;
		if (contains_ci(line, "background")) {
			colon = strchr(line, ':');
			if (colon) {
				char *val = colon + 1;
				while (*val == ' ' || *val == '\t') {
					val++;
				}
				if (*val == '#') {
					unsigned int rgb;
					char hex[7];
					if (strlen(val) >= 7) {
						memcpy(hex, val + 1, 6);
						hex[6] = '\0';
						if (parse_hex_color_value(hex, &rgb) == 0) {
							int dark = color_is_dark(rgb);
							free(copy);
							return dark;
						}
					}
				}
			}
		}
		line = strtok(NULL, "\n");
	}

	free(copy);
	return 0;
}

static void
set_wm_class(Display *dpy, Window win, const char *name)
{
	XClassHint hint;
	char *res_name;
	char *res_class;

	if (!dpy || !win) {
		return;
	}
	if (!name || !*name) {
		name = "s2";
	}
	res_name = strdup(name);
	res_class = strdup(name);
	if (!res_name || !res_class) {
		free(res_name);
		free(res_class);
		return;
	}
	hint.res_name = res_name;
	hint.res_class = res_class;
	XSetClassHint(dpy, win, &hint);
	free(res_name);
	free(res_class);
}

static void
set_wm_window_type(Display *dpy, Window win, int normal_window)
{
	Atom prop;
	Atom kind;

	if (!dpy || !win) {
		return;
	}
	prop = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	if (prop == None) {
		return;
	}
	kind = XInternAtom(dpy,
	                  normal_window ? "_NET_WM_WINDOW_TYPE_NORMAL" : "_NET_WM_WINDOW_TYPE_DIALOG",
	                  False);
	if (kind == None) {
		return;
	}
	XChangeProperty(dpy, win, prop, XA_ATOM, 32, PropModeReplace, (unsigned char *)&kind, 1);
}

static int
img_alloc_like(struct image *dst, const struct image *src)
{
	if (!dst || !src || !src->pixels || src->len == 0) {
		return -1;
	}
	memset(dst, 0, sizeof(*dst));
	dst->width = src->width;
	dst->height = src->height;
	dst->len = src->len;
	dst->pixels = malloc(dst->len);
	if (!dst->pixels) {
		return -1;
	}
	return 0;
}

static void
img_copy(struct image *dst, const struct image *src)
{
	if (!dst || !src || !dst->pixels || !src->pixels || dst->len != src->len) {
		return;
	}
	memcpy(dst->pixels, src->pixels, src->len);
}

static void
img_put_px(struct image *img, int x, int y, unsigned int color)
{
	unsigned char *p;

	if (!img || !img->pixels || x < 0 || y < 0 || x >= img->width || y >= img->height) {
		return;
	}
	p = img->pixels + ((size_t)y * (size_t)img->width + (size_t)x) * 4;
	p[0] = (unsigned char)((color >> 16) & 0xff);
	p[1] = (unsigned char)((color >> 8) & 0xff);
	p[2] = (unsigned char)(color & 0xff);
	p[3] = 0xff;
}

static unsigned long
rgb_to_pixel(unsigned char r, unsigned char g, unsigned char b)
{
	return ((unsigned long)r << 16) | ((unsigned long)g << 8) | (unsigned long)b;
}

static void
render_to_ximg(struct editor_state *ed, const struct image *src)
{
	int x;
	int y;
	int draw_h;
	int draw_w;
	int cw;
	int ch;
	float invs;

	if (!ed || !ed->ximg || !src || !src->pixels) {
		return;
	}
	draw_w = ed->ximg->width;
	draw_h = ed->ximg->height;
	if (draw_h < 1) {
		draw_h = 1;
	}
	cw = ed->canvas_w;
	ch = ed->canvas_h;
	invs = (ed->scale > 0.0f) ? (1.0f / ed->scale) : 1.0f;

	for (y = 0; y < draw_h; y++) {
		for (x = 0; x < draw_w; x++) {
			if (x >= ed->canvas_x && y >= ed->canvas_y && x < ed->canvas_x + cw && y < ed->canvas_y + ch) {
				int sx = (int)((x - ed->canvas_x) * invs);
				int sy = (int)((y - ed->canvas_y) * invs);
				if (sx < 0) sx = 0;
				if (sy < 0) sy = 0;
				if (sx >= src->width) sx = src->width - 1;
				if (sy >= src->height) sy = src->height - 1;
				{
					const unsigned char *p = src->pixels + ((size_t)sy * (size_t)src->width + (size_t)sx) * 4;
					XPutPixel(ed->ximg, x, y, rgb_to_pixel(p[0], p[1], p[2]));
				}
			} else {
				XPutPixel(ed->ximg, x, y, ed->window_bg);
			}
		}
	}
}

static int
recreate_ximg(struct editor_state *ed, Visual *vis, int depth)
{
	int draw_h;
	size_t data_size;

	if (!ed || !ed->dpy || !vis) {
		return -1;
	}
	draw_h = ed->win_h - ed->status_h - ed->status_pad;
	if (draw_h < 1) {
		draw_h = 1;
	}
	if (ed->win_w < 1) {
		ed->win_w = 1;
	}
	if (ed->ximg) {
		XDestroyImage(ed->ximg);
		ed->ximg = NULL;
	}
	data_size = (size_t)ed->win_w * (size_t)draw_h * 4;
	ed->ximg = XCreateImage(ed->dpy,
	                       vis,
	                       (unsigned int)depth,
	                       ZPixmap,
	                       0,
	                       calloc(1, data_size),
	                       ed->win_w,
	                       draw_h,
	                       32,
	                       0);
	if (!ed->ximg || !ed->ximg->data) {
		return -1;
	}
	ed->raster_dirty = 1;
	return 0;
}

static void
recalc_layout(struct editor_state *ed)
{
	int draw_w;
	int draw_h;
	float sx;
	float sy;
	float s;

	if (!ed || !ed->img) {
		return;
	}
	draw_w = ed->win_w - window_padding * 2;
	draw_h = ed->win_h - ed->status_h - ed->status_pad - window_padding * 2;
	if (draw_w < 1) {
		draw_w = 1;
	}
	if (draw_h < 1) {
		draw_h = 1;
	}
	sx = (float)draw_w / (float)ed->img->width;
	sy = (float)draw_h / (float)ed->img->height;
	s = sx < sy ? sx : sy;
	if (s <= 0.0f) {
		s = 1.0f;
	}
	ed->scale = s;
	ed->canvas_w = (int)(ed->img->width * ed->scale);
	ed->canvas_h = (int)(ed->img->height * ed->scale);
	if (ed->canvas_w < 1) ed->canvas_w = 1;
	if (ed->canvas_h < 1) ed->canvas_h = 1;
	ed->canvas_x = window_padding + (draw_w - ed->canvas_w) / 2;
	ed->canvas_y = window_padding + (draw_h - ed->canvas_h) / 2;
	if (ed->canvas_x < window_padding) ed->canvas_x = window_padding;
	if (ed->canvas_y < window_padding) ed->canvas_y = window_padding;
}

static int
canvas_to_image_x(const struct editor_state *ed, int x)
{
	int v;
	if (x < ed->canvas_x) {
		x = ed->canvas_x;
	}
	if (ed->canvas_w <= 0) {
		return 0;
	}
	if (x >= ed->canvas_x + ed->canvas_w) {
		x = ed->canvas_x + ed->canvas_w - 1;
	}
	if (ed->scale <= 0.0f) {
		return 0;
	}
	v = (int)((x - ed->canvas_x) / ed->scale);
	if (v < 0) {
		v = 0;
	}
	if (v >= ed->img->width) {
		v = ed->img->width - 1;
	}
	return v;
}

static int
canvas_to_image_y(const struct editor_state *ed, int y)
{
	int v;
	if (y < ed->canvas_y) {
		y = ed->canvas_y;
	}
	if (ed->canvas_h <= 0) {
		return 0;
	}
	if (y >= ed->canvas_y + ed->canvas_h) {
		y = ed->canvas_y + ed->canvas_h - 1;
	}
	if (ed->scale <= 0.0f) {
		return 0;
	}
	v = (int)((y - ed->canvas_y) / ed->scale);
	if (v < 0) {
		v = 0;
	}
	if (v >= ed->img->height) {
		v = ed->img->height - 1;
	}
	return v;
}

static void
draw_rect_guide(struct image *img, int x0, int y0, int x1, int y1, unsigned int color)
{
	int minx;
	int miny;
	int maxx;
	int maxy;
	int x;
	int y;

	if (!img || !img->pixels) {
		return;
	}
	minx = x0 < x1 ? x0 : x1;
	miny = y0 < y1 ? y0 : y1;
	maxx = x0 > x1 ? x0 : x1;
	maxy = y0 > y1 ? y0 : y1;
	if (minx < 0) minx = 0;
	if (miny < 0) miny = 0;
	if (maxx >= img->width) maxx = img->width - 1;
	if (maxy >= img->height) maxy = img->height - 1;

	for (x = minx; x <= maxx; x++) {
		img_put_px(img, x, miny, color);
		img_put_px(img, x, maxy, color);
	}
	for (y = miny; y <= maxy; y++) {
		img_put_px(img, minx, y, color);
		img_put_px(img, maxx, y, color);
	}
}


static void
action_bounds(const struct editor_state *ed, const struct action *a, int *minx, int *miny, int *maxx, int *maxy)
{
	int x0;
	int y0;
	int x1;
	int y1;
	int i;

	if (!a || !minx || !miny || !maxx || !maxy) {
		if (minx) *minx = 0;
		if (miny) *miny = 0;
		if (maxx) *maxx = 0;
		if (maxy) *maxy = 0;
		return;
	}
	x0 = a->x0 < a->x1 ? a->x0 : a->x1;
	y0 = a->y0 < a->y1 ? a->y0 : a->y1;
	x1 = a->x0 > a->x1 ? a->x0 : a->x1;
	y1 = a->y0 > a->y1 ? a->y0 : a->y1;
	if (a->type == ACTION_TEXT) {
		int bx;
		int by;
		int bw;
		int bh;
		int scale = a->p0 < 0 ? -a->p0 : a->p0;
		x0 = a->x0;
		y0 = a->y0;
		x1 = a->x0;
		y1 = a->y0;
		text_box_metrics(ed, a->x0, a->y0, a->text, scale, a->p0 < 0, &bx, &by, &bw, &bh);
		x0 = bx;
		y0 = by;
		x1 = bx + bw;
		y1 = by + bh;
	} else if (a->type == ACTION_NUMBER) {
		int r = a->p0 > 0 ? a->p0 : 8;
		x0 = a->x0 - r;
		y0 = a->y0 - r;
		x1 = a->x0 + r;
		y1 = a->y0 + r;
	} else if (a->type == ACTION_PEN && a->pen_points && a->pen_len > 0) {
		x0 = x1 = a->pen_points[0];
		y0 = y1 = a->pen_points[1];
		for (i = 1; i < a->pen_len; i++) {
			int px = a->pen_points[i * 2 + 0];
			int py = a->pen_points[i * 2 + 1];
			if (px < x0) x0 = px;
			if (py < y0) y0 = py;
			if (px > x1) x1 = px;
			if (py > y1) y1 = py;
		}
	}
	*minx = x0;
	*miny = y0;
	*maxx = x1;
	*maxy = y1;
}

static void
draw_pen_points(struct image *img, const int *points, int len, int thickness, unsigned int color)
{
	int i;

	if (!img || !points || len <= 0) {
		return;
	}
	if (len == 1) {
		draw_line(img, points[0], points[1], points[0], points[1], thickness, color);
		return;
	}
	for (i = 1; i < len; i++) {
		draw_line(img,
		          points[(i - 1) * 2 + 0],
		          points[(i - 1) * 2 + 1],
		          points[i * 2 + 0],
		          points[i * 2 + 1],
		          thickness,
		          color);
	}
}

static int
text_bg_padding_for_scale(int scale)
{
	if (scale < 1) {
		scale = 1;
	}
	return text_fill_padding * scale;
}

static int
text_bg_radius_for_scale(int scale)
{
	if (scale < 1) {
		scale = 1;
	}
	return text_fill_corner_radius * scale;
}

static void
draw_fill_rounded_rect(struct image *img, int x, int y, int w, int h, int radius, unsigned int color)
{
	int ix;
	int iy;

	if (!img || !img->pixels || w <= 0 || h <= 0) {
		return;
	}
	if (radius < 0) {
		radius = 0;
	}
	if (radius > w / 2) {
		radius = w / 2;
	}
	if (radius > h / 2) {
		radius = h / 2;
	}
	for (iy = 0; iy < h; iy++) {
		for (ix = 0; ix < w; ix++) {
			int draw = 1;
			if (radius > 0) {
				if (ix < radius && iy < radius) {
					int dx = radius - 1 - ix;
					int dy = radius - 1 - iy;
					draw = (dx * dx + dy * dy <= radius * radius);
				} else if (ix >= w - radius && iy < radius) {
					int dx = ix - (w - radius);
					int dy = radius - 1 - iy;
					draw = (dx * dx + dy * dy <= radius * radius);
				} else if (ix < radius && iy >= h - radius) {
					int dx = radius - 1 - ix;
					int dy = iy - (h - radius);
					draw = (dx * dx + dy * dy <= radius * radius);
				} else if (ix >= w - radius && iy >= h - radius) {
					int dx = ix - (w - radius);
					int dy = iy - (h - radius);
					draw = (dx * dx + dy * dy <= radius * radius);
				}
			}
			if (draw) {
				img_put_px(img, x + ix, y + iy, color);
			}
		}
	}
}

static void
text_box_metrics(const struct editor_state *ed,
		 int x,
		 int y,
		 const char *text,
		 int scale,
		 int with_fill,
		 int *bx,
		 int *by,
		 int *bw,
		 int *bh)
{
	int tw;
	int th;
	int pad;
	int ink_x;
	int ink_y;
	int ink_w;
	int ink_h;

	if (scale < 1) {
		scale = 1;
	}
	ink_x = x + TEXT_RENDER_PAD;
	ink_y = y + TEXT_RENDER_PAD;
	ink_w = 0;
	ink_h = 0;
	if (render_text_pango(ed,
	                      NULL,
	                      x,
	                      y,
	                      text ? text : "",
	                      scale,
	                      0,
	                      &ink_x,
	                      &ink_y,
	                      &ink_w,
	                      &ink_h,
	                      1,
	                      &tw,
	                      &th) != 0) {
		text_metrics_xft(ed, text ? text : "", scale, &tw, &th);
		ink_x = x + TEXT_RENDER_PAD;
		ink_y = y + TEXT_RENDER_PAD;
		ink_w = tw;
		ink_h = th;
	}
	pad = with_fill ? text_bg_padding_for_scale(scale) : 0;
	if (bx) {
		*bx = ink_x - pad;
	}
	if (by) {
		*by = ink_y - pad;
	}
	if (bw) {
		*bw = ink_w + pad * 2;
	}
	if (bh) {
		*bh = ink_h + pad * 2;
	}
}

static void
clear_text_mode(struct editor_state *ed)
{
	ed->text_mode = 0;
	ed->text_len = 0;
	ed->text_buf[0] = '\0';
}

static void
commit_text_input(struct editor_state *ed)
{
	if (ed->text_len > 0) {
		struct action a;
		memset(&a, 0, sizeof(a));
		a.type = ACTION_TEXT;
		a.x0 = ed->text_x;
		a.y0 = ed->text_y;
		a.p0 = ed->fill_mode ? -ed->text_scale : ed->text_scale;
		a.color = ed->color;
		a.text = ed->text_buf;
		if (push_action(&ed->actions, &a) == 0) {
			ed->dirty = 1;
			ed->raster_dirty = 1;
		} else {
			fprintf(stderr, "s2: out of memory for text action\n");
		}
	}
	clear_text_mode(ed);
}

static int
append_pen_point(struct editor_state *ed, int x, int y)
{
	int *new_points;

	if (!ed) {
		return -1;
	}
	if (ed->pen_len > 0) {
		int lx = ed->pen_points[(ed->pen_len - 1) * 2 + 0];
		int ly = ed->pen_points[(ed->pen_len - 1) * 2 + 1];
		if (lx == x && ly == y) {
			return 0;
		}
	}
	if (ed->pen_len == ed->pen_cap) {
		int new_cap = ed->pen_cap ? ed->pen_cap * 2 : 64;
		new_points = realloc(ed->pen_points, (size_t)new_cap * 2u * sizeof(*new_points));
		if (!new_points) {
			return -1;
		}
		ed->pen_points = new_points;
		ed->pen_cap = new_cap;
	}
	ed->pen_points[ed->pen_len * 2 + 0] = x;
	ed->pen_points[ed->pen_len * 2 + 1] = y;
	ed->pen_len++;
	return 0;
}

static void
append_pasted_text(struct editor_state *ed, const char *s)
{
	int i;
	if (!ed || !s) {
		return;
	}
	for (i = 0; s[i] != '\0' && ed->text_len < (int)sizeof(ed->text_buf) - 1; i++) {
		unsigned char c = (unsigned char)s[i];
		if (c == '\r' || c == '\n') {
			continue;
		}
		if (c >= 0x20u || c >= 0x80u) {
			ed->text_buf[ed->text_len++] = (char)c;
		}
	}
	ed->text_buf[ed->text_len] = '\0';
}

static int
paste_text_from_clipboard(struct editor_state *ed)
{
	char clip[2048];

	if (!ed) {
		return 0;
	}
	if (clipboard_paste_text(clip, sizeof(clip)) != 0) {
		return 0;
	}
	if (ed->text_mode) {
		append_pasted_text(ed, clip);
		ed->raster_dirty = 1;
		return 1;
	}
	set_tool(ed, TOOL_TEXT);
	ed->text_mode = 1;
	ed->text_x = ed->img->width / 2;
	ed->text_y = ed->img->height / 2;
	ed->text_len = 0;
	ed->text_buf[0] = '\0';
	append_pasted_text(ed, clip);
	ed->raster_dirty = 1;
	return 1;
}

static void
reset_pen_input(struct editor_state *ed)
{
	if (!ed) {
		return;
	}
	ed->pen_active = 0;
	ed->pen_len = 0;
}

static void
draw_text_xft(struct editor_state *ed, struct image *img, int x, int y, const char *text, int scale, unsigned int color)
{
	if (render_text_pango(ed, img, x, y, text, scale, color, NULL, NULL, NULL, NULL, 0, NULL, NULL) != 0) {
		draw_text(img, x, y, text, scale, color);
	}
}

static void
apply_action(struct editor_state *ed, struct image *img, const struct action *a)
{
	if (!ed || !img || !a) {
		return;
	}
	switch (a->type) {
	case ACTION_ARROW:
		draw_arrow(img, a->x0, a->y0, a->x1, a->y1, a->p0, a->color);
		break;
	case ACTION_LINE:
		draw_line(img, a->x0, a->y0, a->x1, a->y1, a->p0, a->color);
		break;
	case ACTION_PEN:
		draw_pen_points(img, a->pen_points, a->pen_len, a->p0, a->color);
		break;
	case ACTION_NUMBER:
		{
			char num[32];
			int scale = a->y1 > 0 ? a->y1 : 1;
			int r = a->p0 > 0 ? a->p0 : number_radius_for_scale(scale);
			int tw;
			int tx;
			int ty;
			unsigned int fg = inverse_color(a->color);
			draw_circle_fill(img, a->x0 - r, a->y0 - r, a->x0 + r, a->y0 + r, a->color);
			draw_circle(img, a->x0 - r, a->y0 - r, a->x0 + r, a->y0 + r, 1, fg);
			snprintf(num, sizeof(num), "%d", a->x1);
			text_metrics_xft(ed, num, scale, &tw, NULL);
			tx = a->x0 - tw / 2;
			ty = a->y0 - 4 * scale;
			draw_text_xft(ed, img, tx, ty, num, scale, fg);
		}
		break;
	case ACTION_RECT:
		if (a->p0 < 0) {
			draw_rect_fill(img, a->x0, a->y0, a->x1, a->y1, a->color);
			draw_rect(img, a->x0, a->y0, a->x1, a->y1, 1, a->color);
		} else {
			draw_rect(img, a->x0, a->y0, a->x1, a->y1, a->p0, a->color);
		}
		break;
	case ACTION_CIRCLE:
		if (a->p0 < 0) {
			draw_circle_fill(img, a->x0, a->y0, a->x1, a->y1, a->color);
			draw_circle(img, a->x0, a->y0, a->x1, a->y1, 1, a->color);
		} else {
			draw_circle(img, a->x0, a->y0, a->x1, a->y1, a->p0, a->color);
		}
		break;
	case ACTION_TEXT:
		{
			int scale = a->p0 < 0 ? -a->p0 : a->p0;
			if (scale < 1) {
				scale = 1;
			}
			if (a->p0 < 0 && a->text) {
				int bx;
				int by;
				int bw;
				int bh;
				int radius;
				unsigned int bg = inverse_color(a->color);
				text_box_metrics(ed, a->x0, a->y0, a->text, scale, 1, &bx, &by, &bw, &bh);
				radius = text_bg_radius_for_scale(scale);
				draw_fill_rounded_rect(img, bx, by, bw, bh, radius, bg);
			}
			draw_text_xft(ed, img, a->x0, a->y0, a->text ? a->text : "", scale, a->color);
		}
		break;
	case ACTION_HIGHLIGHT:
		draw_highlight(img, a->x0, a->y0, a->x1, a->y1, a->color, a->p0);
		break;
	case ACTION_PIXELATE:
		draw_pixelate(img, a->x0, a->y0, a->x1, a->y1, a->p0);
		break;
	case ACTION_BLUR:
		draw_blur(img, a->x0, a->y0, a->x1, a->y1, a->p0);
		break;
	default:
		break;
	}
}

static void
free_action(struct action *a)
{
	if (!a) {
		return;
	}
	if (a->text) {
		free(a->text);
		a->text = NULL;
	}
	if (a->pen_points) {
		free(a->pen_points);
		a->pen_points = NULL;
	}
	a->pen_len = 0;
}

static int
push_action(struct action_stack *st, const struct action *in)
{
	struct action *new_items;
	struct action a;
	size_t i;

	if (!st || !in) {
		return -1;
	}
	if (st->cursor < st->len) {
		for (i = st->cursor; i < st->len; i++) {
			free_action(&st->items[i]);
		}
		st->len = st->cursor;
	}
	if (st->len == st->cap) {
		size_t new_cap = st->cap ? st->cap * 2 : 32;
		new_items = realloc(st->items, new_cap * sizeof(*new_items));
		if (!new_items) {
			return -1;
		}
		st->items = new_items;
		st->cap = new_cap;
	}
	a = *in;
	a.text = NULL;
	a.pen_points = NULL;
	a.pen_len = 0;
	if (in->text) {
		a.text = strdup(in->text);
		if (!a.text) {
			return -1;
		}
	}
	if (in->pen_points && in->pen_len > 0) {
		a.pen_points = malloc((size_t)in->pen_len * 2u * sizeof(*a.pen_points));
		if (!a.pen_points) {
			free(a.text);
			a.text = NULL;
			return -1;
		}
		memcpy(a.pen_points, in->pen_points, (size_t)in->pen_len * 2u * sizeof(*a.pen_points));
		a.pen_len = in->pen_len;
	}
	st->items[st->len++] = a;
	st->cursor = st->len;
	return 0;
}

static int
tool_count(void)
{
	return 12;
}

static enum tool
tool_by_index(int idx)
{
	switch (idx) {
	case 0: return TOOL_SELECT;
	case 1: return TOOL_ARROW;
	case 2: return TOOL_LINE;
	case 3: return TOOL_PEN;
	case 4: return TOOL_NUMBER;
	case 5: return TOOL_RECT;
	case 6: return TOOL_CIRCLE;
	case 7: return TOOL_TEXT;
	case 8: return TOOL_HIGHLIGHT;
	case 9: return TOOL_PIXELATE;
	case 10: return TOOL_BLUR;
	default: return TOOL_PICKER;
	}
}

static const char *
tool_label(enum tool t)
{
	switch (t) {
	case TOOL_SELECT: return "SEL";
	case TOOL_ARROW: return "ARW";
	case TOOL_LINE: return "LIN";
	case TOOL_PEN: return "PEN";
	case TOOL_NUMBER: return "NUM";
	case TOOL_RECT: return "REC";
	case TOOL_CIRCLE: return "CIR";
	case TOOL_TEXT: return "TXT";
	case TOOL_HIGHLIGHT: return "HL";
	case TOOL_PIXELATE: return "PXL";
	case TOOL_BLUR: return "BLR";
	case TOOL_PICKER: return "PCK";
	default: return "?";
	}
}

static int
toolbar_tool_index_at_x(const struct editor_state *ed, int x)
{
	int n;
	int w;
	int idx;

	if (!ed || ed->win_w <= 0) {
		return -1;
	}
	n = tool_count();
	w = ed->win_w / n;
	if (w < 1) {
		return -1;
	}
	idx = x / w;
	if (idx < 0 || idx >= n) {
		return -1;
	}
	return idx;
}

static int
undo_action(struct action_stack *st)
{
	if (!st || st->cursor == 0) {
		return -1;
	}
	st->cursor--;
	return 0;
}

static int
redo_action(struct action_stack *st)
{
	if (!st || st->cursor >= st->len) {
		return -1;
	}
	st->cursor++;
	return 0;
}

static void
free_actions(struct action_stack *st)
{
	size_t i;

	if (!st) {
		return;
	}
	for (i = 0; i < st->len; i++) {
		free_action(&st->items[i]);
	}
	free(st->items);
	st->items = NULL;
	st->len = 0;
	st->cap = 0;
}

static void
rerender_from_actions(struct editor_state *ed)
{
	size_t i;

	img_copy(&ed->rendered, &ed->base);
	for (i = 0; i < ed->actions.cursor; i++) {
		apply_action(ed, &ed->rendered, &ed->actions.items[i]);
	}
	ed->dirty = 0;
}

static void
draw_status(struct editor_state *ed)
{
	char msg[512];
	int i;
	int n;
	int bw;
	int y;
	int by;
	unsigned int bh;
	unsigned long panel_bg;
	unsigned long panel_fg;
	unsigned long panel_hi;

	if (!ed || !ed->dpy || !ed->backbuf || !ed->gc) {
		return;
	}
	snprintf(msg,
	         sizeof(msg),
	         "thickness:%d  text-size:%d  hl-strength:%d  pixelate:%d  blur:%d  fill:%s  selected:%d  actions:%lu/%lu%s%s",
	         thickness_presets[ed->thickness_idx],
	         ed->text_scale,
	         ed->highlight_strength,
	         ed->pixelate_block,
	         ed->blur_radius,
	         ed->fill_mode ? "on" : "off",
	         ed->selected_idx,
	         (unsigned long)ed->actions.cursor,
	         (unsigned long)ed->actions.len,
	         ed->text_mode ? "  text:" : "",
	         ed->text_mode ? ed->text_buf : "");
	panel_bg = ed->status_bg;
	panel_fg = ed->status_fg;
	panel_hi = (ed->status_fg == 0x000000ul) ? 0xddddddul : 0x333333ul;

	bh = (unsigned int)ed->status_h;
	y = ed->win_h - ed->status_h - ed->status_pad;
	XSetForeground(ed->dpy, ed->gc, panel_bg);
	XFillRectangle(ed->dpy, ed->backbuf, ed->gc, 0, y, (unsigned int)ed->win_w, bh);
	n = tool_count();
	bw = ed->win_w / n;
	if (bw < 1) {
		bw = 1;
	}
	for (i = 0; i < n; i++) {
		enum tool tt = tool_by_index(i);
		int x = i * bw;
		if (tt == ed->tool || i == ed->toolbar_hover) {
			XSetForeground(ed->dpy, ed->gc, panel_hi);
			XFillRectangle(ed->dpy, ed->backbuf, ed->gc, x + 1, y + 2, (unsigned int)(bw - 2), bh / 2 - 5);
		}
		XSetForeground(ed->dpy, ed->gc, panel_fg);
		XDrawString(ed->dpy,
		            ed->backbuf,
		            ed->gc,
		            x + 8,
		            y + 15,
		            tool_label(tt),
		            (int)strlen(tool_label(tt)));
		XSetForeground(ed->dpy, ed->gc, (ed->status_fg == 0x000000ul) ? 0x222222ul : 0xddddddul);
		XDrawLine(ed->dpy, ed->backbuf, ed->gc, x + bw, y + 2, x + bw, y + bh / 2 - 3);
	}
	XSetForeground(ed->dpy, ed->gc, (ed->status_fg == 0x000000ul) ? 0x444444ul : 0xbbbbbbul);
	XDrawLine(ed->dpy, ed->backbuf, ed->gc, 0, y + bh / 2 + 1, ed->win_w, y + bh / 2 + 1);
	by = y + bh / 2 + 4;
	XSetForeground(ed->dpy, ed->gc, panel_fg);
	XDrawString(ed->dpy, ed->backbuf, ed->gc, 10, by + 14, msg, (int)strlen(msg));
	{
		XSetForeground(ed->dpy, ed->gc, ed->color & 0xffffff);
		XFillRectangle(ed->dpy, ed->backbuf, ed->gc, ed->win_w - 130, y + bh / 2 + 2, 18, 12);
		XSetForeground(ed->dpy, ed->gc, panel_fg);
		XDrawRectangle(ed->dpy, ed->backbuf, ed->gc, ed->win_w - 130, y + bh / 2 + 2, 18, 12);
	}
}

static void
draw_pen_overlay(struct editor_state *ed)
{
	int i;
	int lw;

	if (!ed || !ed->dpy || !ed->win || !ed->gc || !ed->pen_active || ed->pen_len < 2 || !ed->pen_points) {
		return;
	}
	lw = (int)(ed->pen_thickness * ed->scale);
	if (lw < 1) {
		lw = 1;
	}
	XSetForeground(ed->dpy, ed->gc, (unsigned long)(ed->pen_color & 0xffffff));
	XSetLineAttributes(ed->dpy, ed->gc, (unsigned int)lw, LineSolid, CapRound, JoinRound);
	for (i = 1; i < ed->pen_len; i++) {
		int x0 = ed->canvas_x + (int)(ed->pen_points[(i - 1) * 2 + 0] * ed->scale);
		int y0 = ed->canvas_y + (int)(ed->pen_points[(i - 1) * 2 + 1] * ed->scale);
		int x1 = ed->canvas_x + (int)(ed->pen_points[i * 2 + 0] * ed->scale);
		int y1 = ed->canvas_y + (int)(ed->pen_points[i * 2 + 1] * ed->scale);
		XDrawLine(ed->dpy, ed->win, ed->gc, x0, y0, x1, y1);
	}
	XSetLineAttributes(ed->dpy, ed->gc, 0, LineSolid, CapButt, JoinMiter);
}

static void
draw_help_overlay(struct editor_state *ed)
{
	static const char *lines[] = {
		"s2 key bindings",
		"q / Esc: quit (cancel)",
		"Enter: save and exit",
		"Ctrl+S: save timestamped",
		"Ctrl+C: copy and exit",
		"Ctrl+Y: copy current",
		"Ctrl+V: paste text from clipboard",
		"Ctrl+Z / Ctrl+Shift+Z: undo/redo",
		"s: select  a: arrow  l: line  n: number  r: rect  o: circle",
		"t: text  h: highlight  b: blur  p: pen  x: pixelate  c: picker",
		"Space / left-click: apply tool",
		"arrow keys: move cursor by 1px",
		"[ / ]: adjust size/strength",
		"f: toggle fill  # + 6 hex: set color  1..9: palette color",
		"X: cancel pending anchor/pen/text",
		"?: toggle this help",
	};
	int n = (int)(sizeof(lines) / sizeof(lines[0]));
	int i;
	int pad = 12;
	int line_h = 16;
	int panel_w = 620;
	int panel_h = pad * 2 + line_h * n;
	int x;
	int y;

	if (!ed || !ed->dpy || !ed->win || !ed->gc) {
		return;
	}
	x = (ed->win_w - panel_w) / 2;
	y = (ed->win_h - panel_h) / 2;
	if (x < 10) {
		x = 10;
	}
	if (y < 10) {
		y = 10;
	}
	XSetForeground(ed->dpy, ed->gc, 0x111111ul);
	XFillRectangle(ed->dpy, ed->win, ed->gc, x, y, (unsigned int)panel_w, (unsigned int)panel_h);
	XSetForeground(ed->dpy, ed->gc, 0xeeeeeeul);
	XDrawRectangle(ed->dpy, ed->win, ed->gc, x, y, (unsigned int)panel_w, (unsigned int)panel_h);
	for (i = 0; i < n; i++) {
		XDrawString(ed->dpy,
		            ed->win,
		            ed->gc,
		            x + pad,
		            y + pad + (i + 1) * line_h - 3,
		            lines[i],
		            (int)strlen(lines[i]));
	}
}

static void
render_frame(struct editor_state *ed)
{
	if (ed->dirty) {
		rerender_from_actions(ed);
		ed->raster_dirty = 1;
	}
	if (!ed->ximg || !ed->ximg->data) {
		return;
	}

	img_copy(&ed->preview, &ed->rendered);

	if (ed->anchor_active && !ed->mouse_b1_down) {
		struct action a;
		memset(&a, 0, sizeof(a));
		a.x0 = ed->anchor_x;
		a.y0 = ed->anchor_y;
		a.x1 = ed->cursor_x;
		a.y1 = ed->cursor_y;
		a.color = ed->color;
		switch (ed->tool) {
		case TOOL_ARROW:
			a.type = ACTION_ARROW;
			a.p0 = thickness_presets[ed->thickness_idx];
			apply_action(ed, &ed->preview, &a);
			break;
		case TOOL_LINE:
			a.type = ACTION_LINE;
			a.p0 = thickness_presets[ed->thickness_idx];
			apply_action(ed, &ed->preview, &a);
			break;
		case TOOL_RECT:
			a.type = ACTION_RECT;
			a.p0 = ed->fill_mode ? -1 : thickness_presets[ed->thickness_idx];
			apply_action(ed, &ed->preview, &a);
			draw_rect_guide(&ed->preview, ed->anchor_x, ed->anchor_y, ed->cursor_x, ed->cursor_y, 0xffffff);
			break;
		case TOOL_CIRCLE:
			a.type = ACTION_CIRCLE;
			a.p0 = thickness_presets[ed->thickness_idx];
			apply_action(ed, &ed->preview, &a);
			draw_rect_guide(&ed->preview, ed->anchor_x, ed->anchor_y, ed->cursor_x, ed->cursor_y, 0xffffff);
			break;
		case TOOL_HIGHLIGHT:
			a.type = ACTION_HIGHLIGHT;
			a.p0 = ed->highlight_strength;
			apply_action(ed, &ed->preview, &a);
			draw_rect_guide(&ed->preview, ed->anchor_x, ed->anchor_y, ed->cursor_x, ed->cursor_y, 0xffffff);
			break;
		case TOOL_PIXELATE:
			a.type = ACTION_PIXELATE;
			a.p0 = ed->pixelate_block;
			apply_action(ed, &ed->preview, &a);
			draw_rect_guide(&ed->preview, ed->anchor_x, ed->anchor_y, ed->cursor_x, ed->cursor_y, 0xffffff);
			break;
		case TOOL_BLUR:
			a.type = ACTION_BLUR;
			a.p0 = ed->blur_radius;
			apply_action(ed, &ed->preview, &a);
			draw_rect_guide(&ed->preview, ed->anchor_x, ed->anchor_y, ed->cursor_x, ed->cursor_y, 0xffffff);
			break;
		default:
			break;
		}
	}

	if (ed->text_mode && ed->text_len > 0) {
		if (ed->fill_mode) {
			int bx;
			int by;
			int bw;
			int bh;
			int radius;
			unsigned int bg = inverse_color(ed->color);
			text_box_metrics(ed,
			                 ed->text_x,
			                 ed->text_y,
			                 ed->text_buf,
			                 ed->text_scale > 0 ? ed->text_scale : 1,
			                 1,
			                 &bx,
			                 &by,
			                 &bw,
			                 &bh);
			radius = text_bg_radius_for_scale(ed->text_scale > 0 ? ed->text_scale : 1);
			draw_fill_rounded_rect(&ed->preview, bx, by, bw, bh, radius, bg);
		}
		draw_text_xft(ed, &ed->preview, ed->text_x, ed->text_y, ed->text_buf, ed->text_scale, ed->color);
		ed->raster_dirty = 1;
	}

	if (ed->raster_dirty) {
		render_to_ximg(ed, &ed->preview);
		ed->raster_dirty = 0;
	}

	XSetForeground(ed->dpy, ed->gc, ed->window_bg);
	XFillRectangle(ed->dpy, ed->backbuf, ed->gc, 0, 0, (unsigned int)ed->win_w, (unsigned int)ed->win_h);

	XPutImage(ed->dpy,
	          ed->backbuf,
	          ed->gc,
	          ed->ximg,
	          0,
	          0,
	          0,
	          0,
	          (unsigned int)ed->ximg->width,
	          (unsigned int)ed->ximg->height);
	draw_status(ed);
	XCopyArea(ed->dpy,
	          ed->backbuf,
	          ed->win,
	          ed->gc,
	          0,
	          0,
	          (unsigned int)ed->win_w,
	          (unsigned int)ed->win_h,
	          0,
	          0);
	if (ed->selected_idx >= 0 && (size_t)ed->selected_idx < ed->actions.cursor) {
		struct action *sa = &ed->actions.items[ed->selected_idx];
		int minx;
		int miny;
		int maxx;
		int maxy;
		int sx0;
		int sy0;
		int sx1;
		int sy1;
		action_bounds(ed, sa, &minx, &miny, &maxx, &maxy);
		if (ed->drag_active) {
			minx += ed->drag_dx;
			maxx += ed->drag_dx;
			miny += ed->drag_dy;
			maxy += ed->drag_dy;
		}
		sx0 = ed->canvas_x + (int)(minx * ed->scale);
		sy0 = ed->canvas_y + (int)(miny * ed->scale);
		sx1 = ed->canvas_x + (int)(maxx * ed->scale);
		sy1 = ed->canvas_y + (int)(maxy * ed->scale);
		XSetForeground(ed->dpy, ed->gc, (unsigned long)(selection_bbox_color & 0xffffffu));
		XDrawRectangle(ed->dpy,
		               ed->win,
		               ed->gc,
		               sx0,
		               sy0,
		               (unsigned int)(sx1 >= sx0 ? (sx1 - sx0) : (sx0 - sx1)),
		               (unsigned int)(sy1 >= sy0 ? (sy1 - sy0) : (sy0 - sy1)));
	}
	if (ed->anchor_active && (ed->tool == TOOL_CIRCLE || ed->tool == TOOL_PIXELATE || ed->tool == TOOL_BLUR)) {
		int x0 = ed->canvas_x + (int)(ed->anchor_x * ed->scale);
		int y0 = ed->canvas_y + (int)(ed->anchor_y * ed->scale);
		int x1 = ed->canvas_x + (int)(ed->cursor_x * ed->scale);
		int y1 = ed->canvas_y + (int)(ed->cursor_y * ed->scale);
		int minx = x0 < x1 ? x0 : x1;
		int miny = y0 < y1 ? y0 : y1;
		unsigned int w = (unsigned int)(x0 < x1 ? (x1 - x0) : (x0 - x1));
		unsigned int h = (unsigned int)(y0 < y1 ? (y1 - y0) : (y0 - y1));
		XSetForeground(ed->dpy, ed->gc, ed->status_fg == 0x000000ul ? 0x000000ul : 0xfffffful);
		XDrawRectangle(ed->dpy, ed->win, ed->gc, minx, miny, w, h);
	}
	{
		int cx = ed->canvas_x + (int)(ed->cursor_x * ed->scale);
		int cy = ed->canvas_y + (int)(ed->cursor_y * ed->scale);
		XSetForeground(ed->dpy, ed->gc, 0xfffffful);
		XDrawLine(ed->dpy, ed->win, ed->gc, cx - 8, cy, cx + 8, cy);
		XDrawLine(ed->dpy, ed->win, ed->gc, cx, cy - 8, cx, cy + 8);
		XSetForeground(ed->dpy, ed->gc, 0x000000ul);
		XDrawPoint(ed->dpy, ed->win, ed->gc, cx, cy);
	}
	if (ed->show_help) {
		draw_help_overlay(ed);
	}
	draw_pen_overlay(ed);
	XFlush(ed->dpy);
}

static int
parse_hex_color(const char *s, unsigned int *out)
{
	char *end;
	unsigned long v;

	if (!s || !out || strlen(s) != 6) {
		return -1;
	}
	v = strtoul(s, &end, 16);
	if (*end != '\0' || v > 0xffffff) {
		return -1;
	}
	*out = (unsigned int)v;
	return 0;
}

static void
set_tool(struct editor_state *ed, enum tool tool)
{
	if (!ed) {
		return;
	}
	if (ed->tool == TOOL_SELECT && tool != TOOL_SELECT) {
		ed->selected_idx = -1;
		ed->drag_active = 0;
		ed->drag_dx = 0;
		ed->drag_dy = 0;
	}
	if (ed->tool == TOOL_TEXT && ed->text_mode) {
		commit_text_input(ed);
	}
	if (tool == TOOL_NUMBER && ed->tool != TOOL_NUMBER) {
		ed->number_next = 1;
	}
	ed->tool = tool;
	ed->anchor_active = 0;
	clear_text_mode(ed);
	reset_pen_input(ed);
	ed->color_mode = 0;
}

static int
build_timestamp_name(char *out, size_t outlen)
{
	time_t now;
	struct tm tmv;

	if (!out || outlen < 32) {
		return -1;
	}
	now = time(NULL);
	if (localtime_r(&now, &tmv) == NULL) {
		return -1;
	}
	if (strftime(out, outlen, "%Y-%m-%dT%H%M.png", &tmv) == 0) {
		return -1;
	}
	return 0;
}

static int
build_timestamp_path(const struct app_config *cfg, char *out, size_t outlen)
{
	char name[64];
	char dirbuf[PATH_MAX];
	const char *dir;

	if (!out || outlen < 64) {
		return -1;
	}
	if (build_timestamp_name(name, sizeof(name)) != 0) {
		return -1;
	}
	dir = (cfg && cfg->save_dir_override && cfg->save_dir_override[0] == '/') ? cfg->save_dir_override :
	                                                                           default_save_directory;
	if (!dir || !*dir) {
		dir = ".";
	}
	if (dir[0] == '~' && (dir[1] == '\0' || dir[1] == '/')) {
		const char *home = getenv("HOME");
		if (home && *home) {
			if (dir[1] == '\0') {
				snprintf(dirbuf, sizeof(dirbuf), "%s", home);
			} else {
				snprintf(dirbuf, sizeof(dirbuf), "%s/%s", home, dir + 2);
			}
			dir = dirbuf;
		}
	}
	if (snprintf(out, outlen, "%s/%s", dir, name) >= (int)outlen) {
		return -1;
	}
	return 0;
}

static void
print_saved_abs_path(const char *path)
{
	char resolved[PATH_MAX];
	char cwd[PATH_MAX];

	if (!path || !*path) {
		return;
	}
	if (realpath(path, resolved)) {
		printf("%s\n", resolved);
		fflush(stdout);
		return;
	}
	if (path[0] == '/') {
		printf("%s\n", path);
		fflush(stdout);
		return;
	}
	if (getcwd(cwd, sizeof(cwd))) {
		printf("%s/%s\n", cwd, path);
		fflush(stdout);
		return;
	}
	printf("%s\n", path);
	fflush(stdout);
}

static void
tool_from_toolbar_x(struct editor_state *ed, int x)
{
	int idx;

	if (!ed) {
		return;
	}
	idx = toolbar_tool_index_at_x(ed, x);
	if (idx < 0) {
		return;
	}
	set_tool(ed, tool_by_index(idx));
}

static enum tool
tool_from_config_value(int value)
{
	switch (value) {
	case 0: return TOOL_SELECT;
	case 1: return TOOL_ARROW;
	case 2: return TOOL_LINE;
	case 3: return TOOL_PEN;
	case 4: return TOOL_NUMBER;
	case 5: return TOOL_RECT;
	case 6: return TOOL_CIRCLE;
	case 7: return TOOL_TEXT;
	case 8: return TOOL_HIGHLIGHT;
	case 9: return TOOL_PIXELATE;
	case 10: return TOOL_BLUR;
	case 11: return TOOL_PICKER;
	default: return TOOL_ARROW;
	}
}

static int
add_simple_action(struct editor_state *ed, enum action_type type, int x0, int y0, int x1, int y1, int p0)
{
	struct action a;

	memset(&a, 0, sizeof(a));
	a.type = type;
	a.x0 = x0;
	a.y0 = y0;
	a.x1 = x1;
	a.y1 = y1;
	a.p0 = p0;
	a.color = ed->color;
	if (push_action(&ed->actions, &a) != 0) {
		fprintf(stderr, "s2: out of memory for action stack\n");
		return -1;
	}
	ed->dirty = 1;
	ed->raster_dirty = 1;
	return 0;
}

static int
distance_sq_point_segment(int px, int py, int x0, int y0, int x1, int y1)
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	if (dx == 0 && dy == 0) {
		int ux = px - x0;
		int uy = py - y0;
		return ux * ux + uy * uy;
	}
	{
		double t = ((double)(px - x0) * dx + (double)(py - y0) * dy) / ((double)dx * dx + (double)dy * dy);
		double qx;
		double qy;
		int rx;
		int ry;
		if (t < 0.0) t = 0.0;
		if (t > 1.0) t = 1.0;
		qx = x0 + t * dx;
		qy = y0 + t * dy;
		rx = px - (int)qx;
		ry = py - (int)qy;
		return rx * rx + ry * ry;
	}
}

static int
action_hit_test(const struct editor_state *ed, const struct action *a, int x, int y)
{
	int minx;
	int miny;
	int maxx;
	int maxy;
	int pad;

	if (!a) {
		return 0;
	}
	pad = 6 + a->p0;
	minx = a->x0 < a->x1 ? a->x0 : a->x1;
	miny = a->y0 < a->y1 ? a->y0 : a->y1;
	maxx = a->x0 > a->x1 ? a->x0 : a->x1;
	maxy = a->y0 > a->y1 ? a->y0 : a->y1;

	switch (a->type) {
	case ACTION_ARROW:
	case ACTION_LINE:
		return distance_sq_point_segment(x, y, a->x0, a->y0, a->x1, a->y1) <= pad * pad;
	case ACTION_PEN:
		if (a->pen_points && a->pen_len > 0) {
			int i;
			if (a->pen_len == 1) {
				int ux = x - a->pen_points[0];
				int uy = y - a->pen_points[1];
				return ux * ux + uy * uy <= pad * pad;
			}
			for (i = 1; i < a->pen_len; i++) {
				if (distance_sq_point_segment(x,
				                             y,
				                             a->pen_points[(i - 1) * 2 + 0],
				                             a->pen_points[(i - 1) * 2 + 1],
				                             a->pen_points[i * 2 + 0],
				                             a->pen_points[i * 2 + 1]) <= pad * pad) {
					return 1;
				}
			}
		}
		return 0;
	case ACTION_CIRCLE:
	case ACTION_RECT:
	case ACTION_NUMBER:
	case ACTION_HIGHLIGHT:
	case ACTION_PIXELATE:
	case ACTION_BLUR:
		return x >= minx - pad && x <= maxx + pad && y >= miny - pad && y <= maxy + pad;
	case ACTION_TEXT:
		{
			int bx;
			int by;
			int bw;
			int bh;
			int scale = a->p0 < 0 ? -a->p0 : a->p0;
			int text_pad;
			if (scale < 1) {
				scale = 1;
			}
			text_box_metrics(ed, a->x0, a->y0, a->text ? a->text : "", scale, a->p0 < 0, &bx, &by, &bw, &bh);
			text_pad = pad;
			if (text_pad < 4) {
				text_pad = 4;
			}
			if (a->p0 < 0) {
				int pad_fill = text_bg_padding_for_scale(scale);
				int radius = text_bg_radius_for_scale(scale);
				int left = bx - text_pad;
				int top = by - text_pad;
				int right = bx + bw + text_pad;
				int bottom = by + bh + text_pad;
				int x0 = bx + pad_fill;
				int y0 = by + pad_fill;
				int w = bw - pad_fill * 2;
				int h = bh - pad_fill * 2;
				if (w < 1) {
					w = 1;
				}
				if (h < 1) {
					h = 1;
				}
				if (radius < 0) {
					radius = 0;
				}
				if (radius > w / 2) {
					radius = w / 2;
				}
				if (radius > h / 2) {
					radius = h / 2;
				}
				if (x < left || x > right || y < top || y > bottom) {
					return 0;
				}
				if (radius > 0) {
					if (x < x0 + radius && y < y0 + radius) {
						int dx = (x0 + radius - 1) - x;
						int dy = (y0 + radius - 1) - y;
						return dx * dx + dy * dy <= radius * radius;
					}
					if (x >= x0 + w - radius && y < y0 + radius) {
						int dx = x - (x0 + w - radius);
						int dy = (y0 + radius - 1) - y;
						return dx * dx + dy * dy <= radius * radius;
					}
					if (x < x0 + radius && y >= y0 + h - radius) {
						int dx = (x0 + radius - 1) - x;
						int dy = y - (y0 + h - radius);
						return dx * dx + dy * dy <= radius * radius;
					}
					if (x >= x0 + w - radius && y >= y0 + h - radius) {
						int dx = x - (x0 + w - radius);
						int dy = y - (y0 + h - radius);
						return dx * dx + dy * dy <= radius * radius;
					}
				}
				return 1;
			}
			{
				int extra = scale * 4 + text_bg_padding_for_scale(scale) + TEXT_RENDER_PAD;
				return x >= bx - text_pad - extra && x <= bx + bw + text_pad + extra && y >= by - text_pad - extra &&
				       y <= by + bh + text_pad + extra;
			}
		}
	default:
		return 0;
	}
}

static int
find_action_at(const struct editor_state *ed, int x, int y)
{
	int i;
	for (i = (int)ed->actions.cursor - 1; i >= 0; i--) {
		if (action_hit_test(ed, &ed->actions.items[i], x, y)) {
			return i;
		}
	}
	return -1;
}

static void
action_move_by(struct action *a, int dx, int dy)
{
	int i;

	if (!a) {
		return;
	}
	if (a->type == ACTION_PEN && a->pen_points && a->pen_len > 0) {
		for (i = 0; i < a->pen_len; i++) {
			a->pen_points[i * 2 + 0] += dx;
			a->pen_points[i * 2 + 1] += dy;
		}
		a->x0 += dx;
		a->y0 += dy;
		a->x1 += dx;
		a->y1 += dy;
		return;
	}
	if (a->type == ACTION_NUMBER) {
		a->x0 += dx;
		a->y0 += dy;
		return;
	}
	a->x0 += dx;
	a->y0 += dy;
	a->x1 += dx;
	a->y1 += dy;
}

static int
delete_selected(struct editor_state *ed)
{
	int i;

	if (ed->selected_idx < 0 || (size_t)ed->selected_idx >= ed->actions.cursor) {
		return -1;
	}
	free_action(&ed->actions.items[ed->selected_idx]);
	for (i = ed->selected_idx; (size_t)i + 1 < ed->actions.cursor; i++) {
		ed->actions.items[i] = ed->actions.items[i + 1];
	}
	ed->actions.cursor--;
	ed->actions.len = ed->actions.cursor;
	ed->selected_idx = -1;
	ed->dirty = 1;
	ed->raster_dirty = 1;
	return 0;
}

static int
commit_current_tool(struct editor_state *ed)
{
	switch (ed->tool) {
	case TOOL_SELECT:
		ed->selected_idx = find_action_at(ed, ed->cursor_x, ed->cursor_y);
		return 0;
	case TOOL_ARROW:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_ARROW,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         thickness_presets[ed->thickness_idx]);
	case TOOL_LINE:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_LINE,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         thickness_presets[ed->thickness_idx]);
	case TOOL_PEN:
		return 0;
	case TOOL_NUMBER:
		{
			struct action a;
			int scale = ed->text_scale > 0 ? ed->text_scale : 1;
			int radius = number_radius_for_scale(scale);
			memset(&a, 0, sizeof(a));
			a.type = ACTION_NUMBER;
			a.x0 = ed->cursor_x;
			a.y0 = ed->cursor_y;
			a.x1 = ed->number_next > 0 ? ed->number_next : 1;
			a.y1 = scale;
			a.p0 = radius;
			a.color = ed->color;
			if (push_action(&ed->actions, &a) != 0) {
				fprintf(stderr, "s2: out of memory for number action\n");
				return -1;
			}
			ed->number_next = a.x1 + 1;
			ed->dirty = 1;
			ed->raster_dirty = 1;
			return 0;
		}
	case TOOL_RECT:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_RECT,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         ed->fill_mode ? -1 : thickness_presets[ed->thickness_idx]);
	case TOOL_CIRCLE:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_CIRCLE,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         ed->fill_mode ? -1 : thickness_presets[ed->thickness_idx]);
	case TOOL_PIXELATE:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_PIXELATE,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         ed->pixelate_block);
	case TOOL_BLUR:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_BLUR,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         ed->blur_radius);
	case TOOL_TEXT:
		ed->text_mode = 1;
		ed->text_x = ed->cursor_x;
		ed->text_y = ed->cursor_y;
		ed->text_len = 0;
		ed->text_buf[0] = '\0';
		return 0;
	case TOOL_HIGHLIGHT:
		if (!ed->anchor_active) {
			ed->anchor_active = 1;
			ed->anchor_x = ed->cursor_x;
			ed->anchor_y = ed->cursor_y;
			return 0;
		}
		ed->anchor_active = 0;
		return add_simple_action(ed,
		                         ACTION_HIGHLIGHT,
		                         ed->anchor_x,
		                         ed->anchor_y,
		                         ed->cursor_x,
		                         ed->cursor_y,
		                         ed->highlight_strength);
	case TOOL_PICKER:
		ed->color = draw_sample_color(&ed->rendered, ed->cursor_x, ed->cursor_y);
		return 0;
	default:
		return 0;
	}
}

static int
handle_text_mode(struct editor_state *ed, XKeyEvent *kev)
{
	char buf[64];
	KeySym sym;
	int n;
        Status status;

	sym = XLookupKeysym(kev, 0);
	if (sym == XK_Escape) {
		clear_text_mode(ed);
		ed->suppress_escape_once = 1;
		return 1;
	}
	if (sym == XK_Return) {
		commit_text_input(ed);
		return 1;
	}
	if (sym == XK_BackSpace) {
		if (ed->text_len > 0) {
			ed->text_len--;
			ed->text_buf[ed->text_len] = '\0';
		}
		ed->raster_dirty = 1;
		return 1;
	}
	if ((kev->state & ControlMask) && (sym == XK_v || sym == XK_V)) {
		paste_text_from_clipboard(ed);
		ed->raster_dirty = 1;
		return 1;
	}

	if (ed->xic) {
		n = Xutf8LookupString(ed->xic, kev, buf, (int)sizeof(buf) - 1, &sym, &status);
		if (status == XBufferOverflow) {
			n = 0;
		}
	} else {
		n = XLookupString(kev, buf, (int)sizeof(buf) - 1, NULL, NULL);
	}
	if (n > 0) {
		int i;
		buf[n] = '\0';
		for (i = 0; i < n; i++) {
			unsigned char c = (unsigned char)buf[i];
			if (c == '\r' || c == '\n' || c < 0x20u) {
				continue;
			}
			if (ed->text_len < (int)sizeof(ed->text_buf) - 1) {
				ed->text_buf[ed->text_len++] = (char)c;
				ed->text_buf[ed->text_len] = '\0';
			}
		}
		ed->raster_dirty = 1;
	}
	return 1;
}

static int
handle_color_mode(struct editor_state *ed, XKeyEvent *kev)
{
	char buf[32];
	KeySym sym;
	int n;

	sym = XLookupKeysym(kev, 0);
	if (sym == XK_Escape) {
		ed->color_mode = 0;
		ed->color_len = 0;
		ed->color_buf[0] = '\0';
		return 1;
	}
	if (sym == XK_Return) {
		unsigned int parsed;
		if (parse_hex_color(ed->color_buf, &parsed) == 0) {
			ed->color = parsed;
		}
		ed->color_mode = 0;
		ed->color_len = 0;
		ed->color_buf[0] = '\0';
		return 1;
	}
	if (sym == XK_BackSpace) {
		if (ed->color_len > 0) {
			ed->color_len--;
			ed->color_buf[ed->color_len] = '\0';
		}
		return 1;
	}

	n = XLookupString(kev, buf, (int)sizeof(buf), NULL, NULL);
	if (n > 0) {
		int i;
		for (i = 0; i < n; i++) {
			char c = buf[i];
			if (isxdigit((unsigned char)c) && ed->color_len < 6) {
				ed->color_buf[ed->color_len++] = c;
				ed->color_buf[ed->color_len] = '\0';
			}
		}
	}
	return 1;
}

static void
move_cursor(struct editor_state *ed, int dx, int dy)
{
	ed->cursor_x += dx;
	ed->cursor_y += dy;
	if (ed->cursor_x < 0) {
		ed->cursor_x = 0;
	}
	if (ed->cursor_y < 0) {
		ed->cursor_y = 0;
	}
	if (ed->cursor_x >= ed->img->width) {
		ed->cursor_x = ed->img->width - 1;
	}
	if (ed->cursor_y >= ed->img->height) {
		ed->cursor_y = ed->img->height - 1;
	}
}

static int
tool_uses_anchor(enum tool tool)
{
	return tool == TOOL_ARROW || tool == TOOL_LINE || tool == TOOL_RECT || tool == TOOL_CIRCLE ||
	       tool == TOOL_HIGHLIGHT || tool == TOOL_PIXELATE || tool == TOOL_BLUR;
}

static int
finalize_pending_anchor(struct editor_state *ed)
{
	if (!ed) {
		return -1;
	}
	if (tool_uses_anchor(ed->tool) && ed->anchor_active) {
		return commit_current_tool(ed);
	}
	return 0;
}

static int
handle_keypress(struct editor_state *ed, XKeyEvent *kev)
{
	KeySym sym;
	char kbuf[8];
	int kn;
	int max_t;
	sym = XLookupKeysym(kev, 0);
	kn = XLookupString(kev, kbuf, (int)sizeof(kbuf), NULL, NULL);

	if ((kev->state & ControlMask) && (sym == XK_c || sym == XK_C)) {
		finalize_pending_anchor(ed);
		ed->should_copy = 1;
		ed->running = 0;
		return 1;
	}

	if (ed->text_mode) {
		return handle_text_mode(ed, kev);
	}

	if (sym == XK_question || (kn > 0 && kbuf[0] == '?')) {
		ed->show_help = !ed->show_help;
		return 1;
	}
	if (ed->suppress_escape_once && sym == XK_Escape) {
		ed->suppress_escape_once = 0;
		return 1;
	}
	ed->suppress_escape_once = 0;
	if (ed->color_mode) {
		return handle_color_mode(ed, kev);
	}

	if ((kev->state & ControlMask) && (sym == XK_s || sym == XK_S)) {
		finalize_pending_anchor(ed);
		ed->should_save = 1;
		ed->save_timestamp = 1;
		ed->running = 0;
		return 1;
	}
	if ((kev->state & ControlMask) && (sym == XK_y || sym == XK_Y)) {
		finalize_pending_anchor(ed);
		ed->should_copy = 1;
		return 1;
	}
	if ((kev->state & ControlMask) && (sym == XK_z || sym == XK_Z)) {
		if (kev->state & ShiftMask) {
			if (redo_action(&ed->actions) == 0) {
				ed->dirty = 1;
				ed->raster_dirty = 1;
			}
		} else {
			if (undo_action(&ed->actions) == 0) {
				ed->dirty = 1;
				ed->raster_dirty = 1;
			}
		}
		ed->anchor_active = 0;
		return 1;
	}
	if ((kev->state & ControlMask) && (sym == XK_v || sym == XK_V)) {
		return paste_text_from_clipboard(ed);
	}

	if (sym == XK_q || sym == XK_Escape) {
		ed->cancelled = 1;
		ed->running = 0;
		return 1;
	}

	if (sym == XK_a) {
		set_tool(ed, TOOL_ARROW);
		return 1;
	}
	if (sym == XK_l) {
		set_tool(ed, TOOL_LINE);
		return 1;
	}
	if (sym == XK_p) {
		set_tool(ed, TOOL_PEN);
		return 1;
	}
	if (sym == XK_n) {
		set_tool(ed, TOOL_NUMBER);
		return 1;
	}
	if (sym == XK_r) {
		set_tool(ed, TOOL_RECT);
		return 1;
	}
	if (sym == XK_s) {
		set_tool(ed, TOOL_SELECT);
		return 1;
	}
	if (sym == XK_o || sym == XK_O) {
		set_tool(ed, TOOL_CIRCLE);
		return 1;
	}
	if (sym == XK_t) {
		set_tool(ed, TOOL_TEXT);
		return 1;
	}
	if (sym == XK_h) {
		set_tool(ed, TOOL_HIGHLIGHT);
		return 1;
	}
	if (sym == XK_b) {
		set_tool(ed, TOOL_BLUR);
		return 1;
	}
	if (sym == XK_x) {
		set_tool(ed, TOOL_PIXELATE);
		return 1;
	}
	if (sym == XK_c) {
		set_tool(ed, TOOL_PICKER);
		return 1;
	}

	if (sym == XK_Left) {
		move_cursor(ed, -1, 0);
		return 1;
	}
	if (sym == XK_Right) {
		move_cursor(ed, 1, 0);
		return 1;
	}
	if (sym == XK_Up) {
		move_cursor(ed, 0, -1);
		return 1;
	}
	if (sym == XK_Down) {
		move_cursor(ed, 0, 1);
		return 1;
	}

	if (sym == XK_space) {
		commit_current_tool(ed);
		return 1;
	}
	if (sym == XK_f) {
		ed->fill_mode = !ed->fill_mode;
		ed->raster_dirty = 1;
		return 1;
	}

	if (sym == XK_BackSpace || sym == XK_Delete) {
		delete_selected(ed);
		return 1;
	}

	max_t = (int)(sizeof(thickness_presets) / sizeof(thickness_presets[0])) - 1;
	if (sym == XK_bracketleft) {
		if (ed->tool == TOOL_HIGHLIGHT) {
			if (ed->highlight_strength > 1) {
				ed->highlight_strength -= 1;
			}
		} else if (ed->tool == TOOL_PIXELATE) {
			if (ed->pixelate_block > 2) {
				ed->pixelate_block -= 1;
			}
		} else if (ed->tool == TOOL_BLUR) {
			if (ed->blur_radius > 1) {
				ed->blur_radius -= 1;
			}
		} else if (ed->tool == TOOL_TEXT) {
			if (ed->text_scale > 1) {
				ed->text_scale--;
			}
		} else if (ed->thickness_idx > 0) {
			ed->thickness_idx--;
		}
		return 1;
	}
	if (sym == XK_bracketright) {
		if (ed->tool == TOOL_HIGHLIGHT) {
			if (ed->highlight_strength < 100) {
				ed->highlight_strength += 1;
			}
		} else if (ed->tool == TOOL_PIXELATE) {
			if (ed->pixelate_block < 64) {
				ed->pixelate_block += 1;
			}
		} else if (ed->tool == TOOL_BLUR) {
			if (ed->blur_radius < 16) {
				ed->blur_radius += 1;
			}
		} else if (ed->tool == TOOL_TEXT) {
			if (ed->text_scale < max_text_scale) {
				ed->text_scale++;
			}
		} else if (ed->thickness_idx < max_t) {
			ed->thickness_idx++;
		}
		return 1;
	}

	if (sym >= XK_1 && sym <= XK_9) {
		size_t idx = (size_t)(sym - XK_1);
		size_t n = sizeof(palette) / sizeof(palette[0]);
		if (idx < n) {
			ed->color = palette[idx];
		}
		return 1;
	}

	if (sym == XK_numbersign) {
		ed->color_mode = 1;
		ed->color_len = 0;
		ed->color_buf[0] = '\0';
		return 1;
	}

	if (sym == XK_X) {
		ed->anchor_active = 0;
		reset_pen_input(ed);
		if (ed->text_mode) {
			clear_text_mode(ed);
		}
		ed->raster_dirty = 1;
		return 1;
	}

	return 0;
}

static int
x11_setup(struct editor_state *ed)
{
	Visual *vis;
	int depth;

	ed->dpy = XOpenDisplay(NULL);
	if (!ed->dpy) {
		fprintf(stderr, "s2: cannot open X display\n");
		return -1;
	}
	ed->screen = DefaultScreen(ed->dpy);
	depth = DefaultDepth(ed->dpy, ed->screen);
	vis = DefaultVisual(ed->dpy, ed->screen);
	ed->cmap = DefaultColormap(ed->dpy, ed->screen);
	if (detect_dark_preference(ed->dpy)) {
		ed->window_bg = BlackPixel(ed->dpy, ed->screen);
		ed->status_bg = 0x000000ul;
		ed->status_fg = 0xfffffful;
	} else {
		ed->window_bg = WhitePixel(ed->dpy, ed->screen);
		ed->status_bg = 0xfffffful;
		ed->status_fg = 0x000000ul;
	}

	{
		XSizeHints hints;
		int min_w = 640;
		int min_h = 420;
		int max_w = DisplayWidth(ed->dpy, ed->screen) - window_padding * 2;
		int max_h = DisplayHeight(ed->dpy, ed->screen) - window_padding * 2;
		int pref_w = ed->img->width;
		int pref_h = ed->img->height + ed->status_h + ed->status_pad + window_padding * 2;
		int win_w;
		int win_h;
		if (max_w < min_w) {
			max_w = min_w;
		}
		if (max_h < min_h) {
			max_h = min_h;
		}
		win_w = pref_w > min_w ? pref_w : min_w;
		win_h = pref_h > min_h ? pref_h : min_h;
		if (win_w > max_w) {
			win_w = max_w;
		}
		if (win_h > max_h) {
			win_h = max_h;
		}
		ed->win = XCreateSimpleWindow(ed->dpy,
	                              RootWindow(ed->dpy, ed->screen),
	                              0,
	                              0,
	                              (unsigned int)win_w,
	                              (unsigned int)win_h,
	                              0,
	                              BlackPixel(ed->dpy, ed->screen),
	                              ed->window_bg);
		hints.flags = PMinSize;
		hints.min_width = min_w;
		hints.min_height = min_h;
		XSetWMNormalHints(ed->dpy, ed->win, &hints);
	}
	set_wm_class(ed->dpy,
	             ed->win,
	             (ed->cfg && ed->cfg->window_class && *ed->cfg->window_class) ? ed->cfg->window_class :
	                                                                                 "s2");
	set_wm_window_type(ed->dpy, ed->win, ed->cfg && ed->cfg->normal_window);
	ed->win_w = ed->img->width;
	if (ed->win_w < 640) {
		ed->win_w = 640;
	}
	if (ed->win_w > DisplayWidth(ed->dpy, ed->screen) - window_padding * 2) {
		ed->win_w = DisplayWidth(ed->dpy, ed->screen) - window_padding * 2;
	}
	ed->win_h = ed->img->height + ed->status_h + ed->status_pad + window_padding * 2;
	if (ed->win_h < 420) {
		ed->win_h = 420;
	}
	if (ed->win_h > DisplayHeight(ed->dpy, ed->screen) - window_padding * 2) {
		ed->win_h = DisplayHeight(ed->dpy, ed->screen) - window_padding * 2;
	}
	ed->scale = 1.0f;
	ed->canvas_x = 0;
	ed->canvas_y = 0;
	ed->canvas_w = ed->img->width;
	ed->canvas_h = ed->img->height;
	ed->view_x = 0;
	ed->view_y = 0;
	recalc_layout(ed);
	XStoreName(ed->dpy, ed->win, "s2");
	XSelectInput(ed->dpy,
	             ed->win,
	             ExposureMask | KeyPressMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask |
	                 PointerMotionMask);
	XMapRaised(ed->dpy, ed->win);
	ed->backbuf = XCreatePixmap(ed->dpy,
	                           ed->win,
	                           (unsigned int)ed->win_w,
	                           (unsigned int)ed->win_h,
	                           (unsigned int)depth);
	ed->cursor = XCreateFontCursor(ed->dpy, XC_crosshair);
	if (ed->cursor) {
		XDefineCursor(ed->dpy, ed->win, ed->cursor);
	}

	ed->gc = XCreateGC(ed->dpy, ed->win, 0, NULL);
	if (!ed->gc) {
		fprintf(stderr, "s2: cannot create graphics context\n");
		return -1;
	}

	if (recreate_ximg(ed, vis, depth) != 0) {
		fprintf(stderr, "s2: cannot create X image\n");
		return -1;
	}

	ed->xftdraw = NULL;
	{
		char pat[192];
		build_font_pattern_primary(pat, sizeof(pat), 14);
		ed->xftfont_status = XftFontOpenName(ed->dpy, ed->screen, pat);
		if (!ed->xftfont_status) {
			build_font_pattern(pat, sizeof(pat), 14);
			ed->xftfont_status = XftFontOpenName(ed->dpy, ed->screen, pat);
		}
	}
	if (!ed->xftfont_status) {
		ed->xftfont_status = XftFontOpenName(ed->dpy, ed->screen, "monospace:size=14");
	}
	if (!ed->xftfont_status) {
		fprintf(stderr, "s2: cannot load Xft font\n");
		return -1;
	}

	XSetLocaleModifiers("");
	ed->xim = XOpenIM(ed->dpy, NULL, NULL, NULL);
	if (ed->xim) {
		ed->xic = XCreateIC(ed->xim,
		                   XNInputStyle,
		                   XIMPreeditNothing | XIMStatusNothing,
		                   XNClientWindow,
		                   ed->win,
		                   XNFocusWindow,
		                   ed->win,
		                   NULL);
	}

	return 0;
}

static void
x11_teardown(struct editor_state *ed)
{
	int i;

	if (!ed) {
		return;
	}
	for (i = 0; i < 9; i++) {
		if (ed->xftfont_tool[i] && ed->xftfont_tool[i] != ed->xftfont_status) {
			XftFontClose(ed->dpy, ed->xftfont_tool[i]);
			ed->xftfont_tool[i] = NULL;
		}
	}
	if (ed->xftfont_status) {
		XftFontClose(ed->dpy, ed->xftfont_status);
		ed->xftfont_status = NULL;
	}
	if (ed->cursor) {
		XFreeCursor(ed->dpy, ed->cursor);
		ed->cursor = 0;
	}
	if (ed->xic) {
		XDestroyIC(ed->xic);
		ed->xic = NULL;
	}
	if (ed->xim) {
		XCloseIM(ed->xim);
		ed->xim = NULL;
	}
	if (ed->xftdraw) {
		XftDrawDestroy(ed->xftdraw);
		ed->xftdraw = NULL;
	}
	if (ed->backbuf) {
		XFreePixmap(ed->dpy, ed->backbuf);
		ed->backbuf = 0;
	}
	if (ed->ximg) {
		XDestroyImage(ed->ximg);
		ed->ximg = NULL;
	}
	if (ed->gc) {
		XFreeGC(ed->dpy, ed->gc);
		ed->gc = 0;
	}
	if (ed->win) {
		XDestroyWindow(ed->dpy, ed->win);
		ed->win = 0;
	}
	if (ed->dpy) {
		XCloseDisplay(ed->dpy);
		ed->dpy = NULL;
	}
	image_free(&ed->base);
	image_free(&ed->rendered);
	image_free(&ed->preview);
	free(ed->pen_points);
	ed->pen_points = NULL;
	ed->pen_len = 0;
	ed->pen_cap = 0;
	free_actions(&ed->actions);
}

static void
set_cursor_from_xy(struct editor_state *ed, int x, int y)
{
	ed->cursor_x = canvas_to_image_x(ed, x);
	ed->cursor_y = canvas_to_image_y(ed, y);
	if (ed->cursor_x < 0) {
		ed->cursor_x = 0;
	}
	if (ed->cursor_y < 0) {
		ed->cursor_y = 0;
	}
	if (ed->cursor_x >= ed->img->width) {
		ed->cursor_x = ed->img->width - 1;
	}
	if (ed->cursor_y >= ed->img->height) {
		ed->cursor_y = ed->img->height - 1;
	}
}

static void
handle_button_press(struct editor_state *ed, XButtonEvent *bev)
{
	if (bev->button != Button1) {
		return;
	}
	if (bev->y >= ed->win_h - ed->status_h - ed->status_pad) {
		ed->toolbar_hover = toolbar_tool_index_at_x(ed, bev->x);
		tool_from_toolbar_x(ed, bev->x);
		return;
	}
	set_cursor_from_xy(ed, bev->x, bev->y);
	if (ed->tool == TOOL_TEXT && ed->text_mode) {
		commit_text_input(ed);
		ed->mouse_b1_down = 0;
		return;
	}
	ed->mouse_b1_down = 1;
	ed->mouse_drag_moved = 0;
	if (ed->tool == TOOL_SELECT) {
		ed->selected_idx = find_action_at(ed, ed->cursor_x, ed->cursor_y);
		ed->drag_active = (ed->selected_idx >= 0);
		ed->drag_origin_x = ed->cursor_x;
		ed->drag_origin_y = ed->cursor_y;
		ed->drag_dx = 0;
		ed->drag_dy = 0;
		return;
	}
	if (ed->tool == TOOL_PEN) {
		ed->pen_active = 1;
		ed->pen_color = (int)ed->color;
		ed->pen_thickness = thickness_presets[ed->thickness_idx];
		ed->pen_len = 0;
		if (append_pen_point(ed, ed->cursor_x, ed->cursor_y) != 0) {
			fprintf(stderr, "s2: out of memory for pen tool\n");
			reset_pen_input(ed);
		}
		ed->raster_dirty = 1;
		return;
	}
	if (ed->tool == TOOL_NUMBER) {
		ed->mouse_anchor_set_on_press = 0;
		commit_current_tool(ed);
		return;
	}
	if (tool_uses_anchor(ed->tool)) {
		if (!ed->anchor_active) {
			if (commit_current_tool(ed) == 0) {
				ed->mouse_anchor_set_on_press = 1;
			}
		} else {
			ed->mouse_anchor_set_on_press = 0;
		}
		return;
	}
	ed->mouse_anchor_set_on_press = 0;
	commit_current_tool(ed);
}

static void
handle_button_release(struct editor_state *ed, XButtonEvent *bev)
{
	if (bev->button != Button1) {
		return;
	}
	set_cursor_from_xy(ed, bev->x, bev->y);
	if (!ed->mouse_b1_down) {
		return;
	}
	if (ed->tool == TOOL_SELECT) {
		if (ed->drag_active && (ed->drag_dx != 0 || ed->drag_dy != 0) && ed->selected_idx >= 0 &&
		    (size_t)ed->selected_idx < ed->actions.cursor) {
			action_move_by(&ed->actions.items[ed->selected_idx], ed->drag_dx, ed->drag_dy);
			ed->dirty = 1;
		}
		ed->drag_active = 0;
		ed->drag_dx = 0;
		ed->drag_dy = 0;
		ed->mouse_b1_down = 0;
		ed->mouse_anchor_set_on_press = 0;
		ed->mouse_drag_moved = 0;
		ed->raster_dirty = 1;
		return;
	}
	if (ed->tool == TOOL_PEN) {
		if (ed->pen_active && ed->pen_len > 0) {
			struct action a;
			memset(&a, 0, sizeof(a));
			a.type = ACTION_PEN;
			a.color = (unsigned int)ed->pen_color;
			a.p0 = ed->pen_thickness;
			a.pen_points = ed->pen_points;
			a.pen_len = ed->pen_len;
			a.x0 = ed->pen_points[0];
			a.y0 = ed->pen_points[1];
			a.x1 = ed->pen_points[(ed->pen_len - 1) * 2 + 0];
			a.y1 = ed->pen_points[(ed->pen_len - 1) * 2 + 1];
			if (push_action(&ed->actions, &a) == 0) {
				ed->dirty = 1;
				ed->raster_dirty = 1;
			} else {
				fprintf(stderr, "s2: out of memory for pen action\n");
			}
		}
		reset_pen_input(ed);
		ed->mouse_b1_down = 0;
		ed->mouse_anchor_set_on_press = 0;
		ed->mouse_drag_moved = 0;
		return;
	}
	if (ed->tool == TOOL_TEXT) {
		commit_current_tool(ed);
		ed->mouse_b1_down = 0;
		ed->mouse_anchor_set_on_press = 0;
		ed->mouse_drag_moved = 0;
		return;
	}
	if (tool_uses_anchor(ed->tool) && ed->anchor_active) {
		if (!ed->mouse_anchor_set_on_press || ed->mouse_drag_moved) {
			commit_current_tool(ed);
		}
	}
	ed->mouse_b1_down = 0;
	ed->mouse_anchor_set_on_press = 0;
	ed->mouse_drag_moved = 0;
}

static void
handle_motion(struct editor_state *ed, XMotionEvent *mev)
{
	int oldx;
	int oldy;

	oldx = ed->cursor_x;
	oldy = ed->cursor_y;
	set_cursor_from_xy(ed, mev->x, mev->y);
	if (ed->tool == TOOL_SELECT && ed->drag_active && ed->selected_idx >= 0 &&
	    (size_t)ed->selected_idx < ed->actions.cursor) {
		int dx = ed->cursor_x - ed->drag_origin_x;
		int dy = ed->cursor_y - ed->drag_origin_y;
		if (dx != 0 || dy != 0) {
			ed->drag_dx = dx;
			ed->drag_dy = dy;
		}
	}
	if (ed->tool == TOOL_PEN && ed->pen_active) {
		if (append_pen_point(ed, ed->cursor_x, ed->cursor_y) != 0) {
			fprintf(stderr, "s2: out of memory for pen tool\n");
			reset_pen_input(ed);
		}
	}
	if (ed->mouse_b1_down && (ed->cursor_x != oldx || ed->cursor_y != oldy)) {
		ed->mouse_drag_moved = 1;
	}
	if (mev->y >= ed->win_h - ed->status_h - ed->status_pad) {
		ed->toolbar_hover = toolbar_tool_index_at_x(ed, mev->x);
	} else {
		ed->toolbar_hover = -1;
	}
}

int
editor_run(const struct app_config *cfg, struct image *img)
{
	struct editor_state ed;
	XEvent ev;
	(void)default_window_class;

	if (!cfg || !img || !img->pixels || img->width <= 0 || img->height <= 0) {
		fprintf(stderr, "s2: invalid editor input\n");
		return -1;
	}

	memset(&ed, 0, sizeof(ed));
	ed.cfg = cfg;
	ed.img = img;
	ed.tool = tool_from_config_value(default_tool_index);
	{
		size_t pn = sizeof(palette) / sizeof(palette[0]);
		size_t tn = sizeof(thickness_presets) / sizeof(thickness_presets[0]);
		int pidx = default_palette_index;
		int tidx = default_thickness_index;
		if (pidx < 0 || (size_t)pidx >= pn) {
			pidx = 0;
		}
		if (tidx < 0 || (size_t)tidx >= tn) {
			tidx = 0;
		}
		ed.color = palette[pidx];
		ed.thickness_idx = tidx;
	}
	ed.text_scale = (default_text_scale > 0) ? default_text_scale : 1;
	if (ed.text_scale > max_text_scale) {
		ed.text_scale = max_text_scale;
	}
	ed.pixelate_block = default_pixelate_block;
	if (ed.pixelate_block < 2) {
		ed.pixelate_block = 2;
	}
	if (ed.pixelate_block > 64) {
		ed.pixelate_block = 64;
	}
	ed.blur_radius = default_blur_radius;
	if (ed.blur_radius < 1) {
		ed.blur_radius = 1;
	}
	if (ed.blur_radius > 16) {
		ed.blur_radius = 16;
	}
	ed.highlight_strength = default_highlight_strength;
	if (ed.highlight_strength < 1) {
		ed.highlight_strength = 1;
	}
	if (ed.highlight_strength > 100) {
		ed.highlight_strength = 100;
	}
	ed.fill_mode = default_fill_mode ? 1 : 0;
	ed.status_h = 28;
	ed.status_pad = 10;
	ed.scale = 1.0f;
	ed.cursor_x = img->width / 2;
	ed.cursor_y = img->height / 2;
	ed.running = 1;
	ed.dirty = 1;
	ed.raster_dirty = 1;
	ed.selected_idx = -1;
	ed.toolbar_hover = -1;
	ed.save_timestamp = 0;
	ed.number_next = 1;

	if (img_alloc_like(&ed.base, img) != 0 ||
	    img_alloc_like(&ed.rendered, img) != 0 ||
	    img_alloc_like(&ed.preview, img) != 0) {
		fprintf(stderr, "s2: out of memory for working buffers\n");
		x11_teardown(&ed);
		return -1;
	}
	img_copy(&ed.base, img);

	if (x11_setup(&ed) != 0) {
		x11_teardown(&ed);
		return -1;
	}
	setlocale(LC_CTYPE, "");
	setlocale(LC_ALL, "");
	if (ed.xic) {
		XSetICFocus(ed.xic);
	}

	render_frame(&ed);
	while (ed.running) {
		XNextEvent(ed.dpy, &ev);
		switch (ev.type) {
		case Expose:
		case ConfigureNotify:
			if (ev.type == ConfigureNotify) {
				Visual *vis;
				int depth;
				vis = DefaultVisual(ed.dpy, ed.screen);
				depth = DefaultDepth(ed.dpy, ed.screen);
				ed.win_w = ev.xconfigure.width;
				ed.win_h = ev.xconfigure.height;
				recalc_layout(&ed);
				ed.view_x = 0;
				ed.view_y = 0;
				if (ed.backbuf) {
					XFreePixmap(ed.dpy, ed.backbuf);
					ed.backbuf = XCreatePixmap(ed.dpy,
					                           ed.win,
					                           (unsigned int)ed.win_w,
					                           (unsigned int)ed.win_h,
					                           (unsigned int)DefaultDepth(ed.dpy, ed.screen));
				}
				recreate_ximg(&ed, vis, depth);
				ed.raster_dirty = 1;
			}
			render_frame(&ed);
			break;
		case KeyPress:
			handle_keypress(&ed, &ev.xkey);
			render_frame(&ed);
			break;
		case ButtonPress:
			handle_button_press(&ed, &ev.xbutton);
			render_frame(&ed);
			break;
		case ButtonRelease:
			handle_button_release(&ed, &ev.xbutton);
			render_frame(&ed);
			break;
		case MotionNotify:
			handle_motion(&ed, &ev.xmotion);
			if (ed.drag_active || ed.anchor_active || ed.text_mode || ed.tool == TOOL_PICKER ||
			    ev.xmotion.y >= ed.win_h - ed.status_h) {
				render_frame(&ed);
			}
			break;
		default:
			break;
		}
	}

	if (ed.dirty) {
		rerender_from_actions(&ed);
	}
	img_copy(img, &ed.rendered);

	if (ed.should_save) {
		char tsname[PATH_MAX];
		const char *outpath = cfg->output_path;
		if (ed.save_timestamp && build_timestamp_path(cfg, tsname, sizeof(tsname)) == 0) {
			outpath = tsname;
		}
		if (image_save_png(img, outpath) != 0) {
			x11_teardown(&ed);
			return -1;
		}
		print_saved_abs_path(outpath);
	}

	if (ed.should_copy || cfg->copy_on_finish) {
		if (clipboard_copy_png(img) != 0) {
			x11_teardown(&ed);
			return -1;
		}
	}
	if (ed.cancelled) {
		x11_teardown(&ed);
		return -1;
	}

	x11_teardown(&ed);
	return 0;
}
