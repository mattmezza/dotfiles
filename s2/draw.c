#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "s2.h"

static void
color_to_rgb(unsigned int color, unsigned char *r, unsigned char *g, unsigned char *b)
{
	*r = (unsigned char)((color >> 16) & 0xff);
	*g = (unsigned char)((color >> 8) & 0xff);
	*b = (unsigned char)(color & 0xff);
}

static int
in_bounds(const struct image *img, int x, int y)
{
	return img && img->pixels && x >= 0 && y >= 0 && x < img->width && y < img->height;
}

static void
put_px(struct image *img, int x, int y, unsigned int color)
{
	unsigned char *p;
	unsigned char r;
	unsigned char g;
	unsigned char b;

	if (!in_bounds(img, x, y)) {
		return;
	}
	color_to_rgb(color, &r, &g, &b);
	p = img->pixels + ((size_t)y * (size_t)img->width + (size_t)x) * 4;
	p[0] = r;
	p[1] = g;
	p[2] = b;
	p[3] = 0xff;
}

static void
draw_disk(struct image *img, int cx, int cy, int radius, unsigned int color)
{
	int x;
	int y;
	int rr;

	if (radius < 1) {
		radius = 1;
	}
	rr = radius * radius;
	for (y = -radius; y <= radius; y++) {
		for (x = -radius; x <= radius; x++) {
			if ((x * x + y * y) <= rr) {
				put_px(img, cx + x, cy + y, color);
			}
		}
	}
}

static void
draw_line_thick(struct image *img, int x0, int y0, int x1, int y1, int thickness, unsigned int color)
{
	int steps;
	int i;
	double dx;
	double dy;
	double x;
	double y;

	if (thickness < 1) {
		thickness = 1;
	}
	dx = (double)(x1 - x0);
	dy = (double)(y1 - y0);
	steps = (int)fmax(fabs(dx), fabs(dy));
	if (steps < 1) {
		draw_disk(img, x0, y0, thickness / 2 + 1, color);
		return;
	}
	x = (double)x0;
	y = (double)y0;
	for (i = 0; i <= steps; i++) {
		draw_disk(img, (int)lround(x), (int)lround(y), thickness / 2 + 1, color);
		x += dx / (double)steps;
		y += dy / (double)steps;
	}
}

void
draw_line(struct image *img, int x0, int y0, int x1, int y1, int thickness, unsigned int color)
{
	draw_line_thick(img, x0, y0, x1, y1, thickness, color);
}

void
draw_arrow(struct image *img, int x0, int y0, int x1, int y1, int thickness, unsigned int color)
{
	double angle;
	double head_len;
	double left;
	double right;
	int hx1;
	int hy1;
	int hx2;
	int hy2;

	draw_line_thick(img, x0, y0, x1, y1, thickness, color);
	angle = atan2((double)(y1 - y0), (double)(x1 - x0));
	head_len = 10.0 + (double)thickness * 2.0;
	left = angle + 2.6;
	right = angle - 2.6;
	hx1 = x1 + (int)lround(cos(left) * head_len);
	hy1 = y1 + (int)lround(sin(left) * head_len);
	hx2 = x1 + (int)lround(cos(right) * head_len);
	hy2 = y1 + (int)lround(sin(right) * head_len);
	draw_line_thick(img, x1, y1, hx1, hy1, thickness, color);
	draw_line_thick(img, x1, y1, hx2, hy2, thickness, color);
}

void
draw_circle(struct image *img, int x0, int y0, int x1, int y1, int thickness, unsigned int color)
{
	int minx;
	int miny;
	int maxx;
	int maxy;
	int w;
	int h;
	double cx;
	double cy;
	double rx;
	double ry;
	int i;
	int samples;

	if (thickness < 1) {
		thickness = 1;
	}
	minx = x0 < x1 ? x0 : x1;
	miny = y0 < y1 ? y0 : y1;
	maxx = x0 > x1 ? x0 : x1;
	maxy = y0 > y1 ? y0 : y1;
	w = maxx - minx;
	h = maxy - miny;
	if (w < 2 || h < 2) {
		draw_disk(img, x0, y0, thickness / 2 + 1, color);
		return;
	}
	cx = (double)(minx + maxx) / 2.0;
	cy = (double)(miny + maxy) / 2.0;
	rx = (double)w / 2.0;
	ry = (double)h / 2.0;
	samples = (int)(2.0 * M_PI * (rx > ry ? rx : ry));
	if (samples < 60) {
		samples = 60;
	}
	for (i = 0; i < samples; i++) {
		double a = ((double)i / (double)samples) * 2.0 * M_PI;
		int px = (int)lround(cx + cos(a) * rx);
		int py = (int)lround(cy + sin(a) * ry);
		draw_disk(img, px, py, thickness / 2 + 1, color);
	}
}

void
draw_circle_fill(struct image *img, int x0, int y0, int x1, int y1, unsigned int color)
{
	int minx;
	int miny;
	int maxx;
	int maxy;
	double cx;
	double cy;
	double rx;
	double ry;
	int x;
	int y;

	minx = x0 < x1 ? x0 : x1;
	miny = y0 < y1 ? y0 : y1;
	maxx = x0 > x1 ? x0 : x1;
	maxy = y0 > y1 ? y0 : y1;
	if (minx == maxx || miny == maxy) {
		put_px(img, x0, y0, color);
		return;
	}
	cx = (double)(minx + maxx) / 2.0;
	cy = (double)(miny + maxy) / 2.0;
	rx = (double)(maxx - minx) / 2.0;
	ry = (double)(maxy - miny) / 2.0;
	if (rx < 1.0) rx = 1.0;
	if (ry < 1.0) ry = 1.0;

	for (y = miny; y <= maxy; y++) {
		for (x = minx; x <= maxx; x++) {
			double nx = ((double)x - cx) / rx;
			double ny = ((double)y - cy) / ry;
			if (nx * nx + ny * ny <= 1.0) {
				put_px(img, x, y, color);
			}
		}
	}
}

void
draw_rect(struct image *img, int x0, int y0, int x1, int y1, int thickness, unsigned int color)
{
	int minx = x0 < x1 ? x0 : x1;
	int miny = y0 < y1 ? y0 : y1;
	int maxx = x0 > x1 ? x0 : x1;
	int maxy = y0 > y1 ? y0 : y1;
	int t;

	if (thickness < 1) {
		thickness = 1;
	}
	for (t = 0; t < thickness; t++) {
		draw_line_thick(img, minx + t, miny + t, maxx - t, miny + t, 1, color);
		draw_line_thick(img, minx + t, maxy - t, maxx - t, maxy - t, 1, color);
		draw_line_thick(img, minx + t, miny + t, minx + t, maxy - t, 1, color);
		draw_line_thick(img, maxx - t, miny + t, maxx - t, maxy - t, 1, color);
	}
}

void
draw_rect_fill(struct image *img, int x0, int y0, int x1, int y1, unsigned int color)
{
	int minx = x0 < x1 ? x0 : x1;
	int miny = y0 < y1 ? y0 : y1;
	int maxx = x0 > x1 ? x0 : x1;
	int maxy = y0 > y1 ? y0 : y1;
	int x;
	int y;

	for (y = miny; y <= maxy; y++) {
		for (x = minx; x <= maxx; x++) {
			put_px(img, x, y, color);
		}
	}
}

void
draw_highlight(struct image *img, int x0, int y0, int x1, int y1, unsigned int color, int strength)
{
	int minx = x0 < x1 ? x0 : x1;
	int miny = y0 < y1 ? y0 : y1;
	int maxx = x0 > x1 ? x0 : x1;
	int maxy = y0 > y1 ? y0 : y1;
	int x;
	int y;
	unsigned int r = (color >> 16) & 0xffu;
	unsigned int g = (color >> 8) & 0xffu;
	unsigned int b = color & 0xffu;
	unsigned int s;
	unsigned int inv;

	if (strength < 1) {
		strength = 1;
	}
	if (strength > 100) {
		strength = 100;
	}
	s = (unsigned int)strength;
	inv = 100u - s;

	for (y = miny; y <= maxy; y++) {
		for (x = minx; x <= maxx; x++) {
			if (in_bounds(img, x, y)) {
				unsigned char *p = img->pixels + ((size_t)y * (size_t)img->width + (size_t)x) * 4;
				p[0] = (unsigned char)(((unsigned int)p[0] * inv + r * s) / 100u);
				p[1] = (unsigned char)(((unsigned int)p[1] * inv + g * s) / 100u);
				p[2] = (unsigned char)(((unsigned int)p[2] * inv + b * s) / 100u);
				p[3] = 0xff;
			}
		}
	}
}

static int
glyph5x7(char ch, unsigned char out[7])
{
	char c = (char)toupper((unsigned char)ch);
	memset(out, 0, 7);
	switch (c) {
	case 'A': out[0]=0x0e; out[1]=0x11; out[2]=0x11; out[3]=0x1f; out[4]=0x11; out[5]=0x11; out[6]=0x11; break;
	case 'B': out[0]=0x1e; out[1]=0x11; out[2]=0x11; out[3]=0x1e; out[4]=0x11; out[5]=0x11; out[6]=0x1e; break;
	case 'C': out[0]=0x0e; out[1]=0x11; out[2]=0x10; out[3]=0x10; out[4]=0x10; out[5]=0x11; out[6]=0x0e; break;
	case 'D': out[0]=0x1c; out[1]=0x12; out[2]=0x11; out[3]=0x11; out[4]=0x11; out[5]=0x12; out[6]=0x1c; break;
	case 'E': out[0]=0x1f; out[1]=0x10; out[2]=0x10; out[3]=0x1e; out[4]=0x10; out[5]=0x10; out[6]=0x1f; break;
	case 'F': out[0]=0x1f; out[1]=0x10; out[2]=0x10; out[3]=0x1e; out[4]=0x10; out[5]=0x10; out[6]=0x10; break;
	case 'G': out[0]=0x0f; out[1]=0x10; out[2]=0x10; out[3]=0x17; out[4]=0x11; out[5]=0x11; out[6]=0x0f; break;
	case 'H': out[0]=0x11; out[1]=0x11; out[2]=0x11; out[3]=0x1f; out[4]=0x11; out[5]=0x11; out[6]=0x11; break;
	case 'I': out[0]=0x1f; out[1]=0x04; out[2]=0x04; out[3]=0x04; out[4]=0x04; out[5]=0x04; out[6]=0x1f; break;
	case 'J': out[0]=0x01; out[1]=0x01; out[2]=0x01; out[3]=0x01; out[4]=0x11; out[5]=0x11; out[6]=0x0e; break;
	case 'K': out[0]=0x11; out[1]=0x12; out[2]=0x14; out[3]=0x18; out[4]=0x14; out[5]=0x12; out[6]=0x11; break;
	case 'L': out[0]=0x10; out[1]=0x10; out[2]=0x10; out[3]=0x10; out[4]=0x10; out[5]=0x10; out[6]=0x1f; break;
	case 'M': out[0]=0x11; out[1]=0x1b; out[2]=0x15; out[3]=0x15; out[4]=0x11; out[5]=0x11; out[6]=0x11; break;
	case 'N': out[0]=0x11; out[1]=0x19; out[2]=0x15; out[3]=0x13; out[4]=0x11; out[5]=0x11; out[6]=0x11; break;
	case 'O': out[0]=0x0e; out[1]=0x11; out[2]=0x11; out[3]=0x11; out[4]=0x11; out[5]=0x11; out[6]=0x0e; break;
	case 'P': out[0]=0x1e; out[1]=0x11; out[2]=0x11; out[3]=0x1e; out[4]=0x10; out[5]=0x10; out[6]=0x10; break;
	case 'Q': out[0]=0x0e; out[1]=0x11; out[2]=0x11; out[3]=0x11; out[4]=0x15; out[5]=0x12; out[6]=0x0d; break;
	case 'R': out[0]=0x1e; out[1]=0x11; out[2]=0x11; out[3]=0x1e; out[4]=0x14; out[5]=0x12; out[6]=0x11; break;
	case 'S': out[0]=0x0f; out[1]=0x10; out[2]=0x10; out[3]=0x0e; out[4]=0x01; out[5]=0x01; out[6]=0x1e; break;
	case 'T': out[0]=0x1f; out[1]=0x04; out[2]=0x04; out[3]=0x04; out[4]=0x04; out[5]=0x04; out[6]=0x04; break;
	case 'U': out[0]=0x11; out[1]=0x11; out[2]=0x11; out[3]=0x11; out[4]=0x11; out[5]=0x11; out[6]=0x0e; break;
	case 'V': out[0]=0x11; out[1]=0x11; out[2]=0x11; out[3]=0x11; out[4]=0x11; out[5]=0x0a; out[6]=0x04; break;
	case 'W': out[0]=0x11; out[1]=0x11; out[2]=0x11; out[3]=0x15; out[4]=0x15; out[5]=0x1b; out[6]=0x11; break;
	case 'X': out[0]=0x11; out[1]=0x11; out[2]=0x0a; out[3]=0x04; out[4]=0x0a; out[5]=0x11; out[6]=0x11; break;
	case 'Y': out[0]=0x11; out[1]=0x11; out[2]=0x0a; out[3]=0x04; out[4]=0x04; out[5]=0x04; out[6]=0x04; break;
	case 'Z': out[0]=0x1f; out[1]=0x01; out[2]=0x02; out[3]=0x04; out[4]=0x08; out[5]=0x10; out[6]=0x1f; break;
	case '0': out[0]=0x0e; out[1]=0x11; out[2]=0x13; out[3]=0x15; out[4]=0x19; out[5]=0x11; out[6]=0x0e; break;
	case '1': out[0]=0x04; out[1]=0x0c; out[2]=0x04; out[3]=0x04; out[4]=0x04; out[5]=0x04; out[6]=0x0e; break;
	case '2': out[0]=0x0e; out[1]=0x11; out[2]=0x01; out[3]=0x02; out[4]=0x04; out[5]=0x08; out[6]=0x1f; break;
	case '3': out[0]=0x1e; out[1]=0x01; out[2]=0x01; out[3]=0x0e; out[4]=0x01; out[5]=0x01; out[6]=0x1e; break;
	case '4': out[0]=0x02; out[1]=0x06; out[2]=0x0a; out[3]=0x12; out[4]=0x1f; out[5]=0x02; out[6]=0x02; break;
	case '5': out[0]=0x1f; out[1]=0x10; out[2]=0x10; out[3]=0x1e; out[4]=0x01; out[5]=0x01; out[6]=0x1e; break;
	case '6': out[0]=0x0e; out[1]=0x10; out[2]=0x10; out[3]=0x1e; out[4]=0x11; out[5]=0x11; out[6]=0x0e; break;
	case '7': out[0]=0x1f; out[1]=0x01; out[2]=0x02; out[3]=0x04; out[4]=0x08; out[5]=0x08; out[6]=0x08; break;
	case '8': out[0]=0x0e; out[1]=0x11; out[2]=0x11; out[3]=0x0e; out[4]=0x11; out[5]=0x11; out[6]=0x0e; break;
	case '9': out[0]=0x0e; out[1]=0x11; out[2]=0x11; out[3]=0x0f; out[4]=0x01; out[5]=0x01; out[6]=0x0e; break;
	case '.': out[6]=0x04; break;
	case ',': out[5]=0x04; out[6]=0x08; break;
	case ':': out[2]=0x04; out[5]=0x04; break;
	case ';': out[2]=0x04; out[5]=0x04; out[6]=0x08; break;
	case '!': out[0]=0x04; out[1]=0x04; out[2]=0x04; out[3]=0x04; out[6]=0x04; break;
	case '?': out[0]=0x0e; out[1]=0x11; out[2]=0x01; out[3]=0x02; out[4]=0x04; out[6]=0x04; break;
	case '-': out[3]=0x0e; break;
	case '_': out[6]=0x1f; break;
	case '/': out[0]=0x01; out[1]=0x02; out[2]=0x04; out[3]=0x08; out[4]=0x10; break;
	case '(': out[1]=0x02; out[2]=0x04; out[3]=0x04; out[4]=0x04; out[5]=0x02; break;
	case ')': out[1]=0x08; out[2]=0x04; out[3]=0x04; out[4]=0x04; out[5]=0x08; break;
	case ' ': break;
	default:
		out[0]=0x0e; out[1]=0x11; out[2]=0x02; out[3]=0x04; out[4]=0x00; out[5]=0x04; out[6]=0x00;
		break;
	}
	return 0;
}

static void
draw_char_5x7(struct image *img, int x, int y, char ch, int scale, unsigned int color)
{
	unsigned char rows[7];
	int ry;
	int rx;
	int sy;
	int sx;

	if (scale < 1) {
		scale = 1;
	}
	glyph5x7(ch, rows);
	for (ry = 0; ry < 7; ry++) {
		for (rx = 0; rx < 5; rx++) {
			if ((rows[ry] >> (4 - rx)) & 1) {
				for (sy = 0; sy < scale; sy++) {
					for (sx = 0; sx < scale; sx++) {
						put_px(img, x + rx * scale + sx, y + ry * scale + sy, color);
					}
				}
			}
		}
	}
}

void
draw_text(struct image *img, int x, int y, const char *text, int scale, unsigned int color)
{
	int cx;
	int cy;
	size_t i;

	if (!img || !text) {
		return;
	}
	if (scale < 1) {
		scale = 1;
	}
	cx = x;
	cy = y;
	for (i = 0; text[i] != '\0'; i++) {
		if (text[i] == '\n') {
			cx = x;
			cy += 9 * scale;
			continue;
		}
		draw_char_5x7(img, cx, cy, text[i], scale, color);
		cx += 6 * scale;
	}
}

void
draw_pixelate(struct image *img, int x0, int y0, int x1, int y1, int block)
{
	int minx;
	int miny;
	int maxx;
	int maxy;
	int x;
	int y;
	int bx;
	int by;
	unsigned int rsum;
	unsigned int gsum;
	unsigned int bsum;
	unsigned int count;

	if (!img || !img->pixels) {
		return;
	}
	if (block < 2) {
		block = 2;
	}
	minx = x0 < x1 ? x0 : x1;
	miny = y0 < y1 ? y0 : y1;
	maxx = x0 > x1 ? x0 : x1;
	maxy = y0 > y1 ? y0 : y1;
	if (minx < 0) minx = 0;
	if (miny < 0) miny = 0;
	if (maxx >= img->width) maxx = img->width - 1;
	if (maxy >= img->height) maxy = img->height - 1;

	for (by = miny; by <= maxy; by += block) {
		for (bx = minx; bx <= maxx; bx += block) {
			rsum = gsum = bsum = count = 0;
			for (y = by; y < by + block && y <= maxy; y++) {
				for (x = bx; x < bx + block && x <= maxx; x++) {
					unsigned char *p = img->pixels + ((size_t)y * (size_t)img->width + (size_t)x) * 4;
					rsum += p[0];
					gsum += p[1];
					bsum += p[2];
					count++;
				}
			}
			if (count == 0) {
				continue;
			}
			for (y = by; y < by + block && y <= maxy; y++) {
				for (x = bx; x < bx + block && x <= maxx; x++) {
					unsigned char *p = img->pixels + ((size_t)y * (size_t)img->width + (size_t)x) * 4;
					p[0] = (unsigned char)(rsum / count);
					p[1] = (unsigned char)(gsum / count);
					p[2] = (unsigned char)(bsum / count);
					p[3] = 0xff;
				}
			}
		}
	}
}

void
draw_blur(struct image *img, int x0, int y0, int x1, int y1, int radius)
{
	int minx;
	int miny;
	int maxx;
	int maxy;
	int x;
	int y;
	int dx;
	int dy;
	unsigned char *tmp;

	if (!img || !img->pixels) {
		return;
	}
	if (radius < 1) {
		radius = 1;
	}
	minx = x0 < x1 ? x0 : x1;
	miny = y0 < y1 ? y0 : y1;
	maxx = x0 > x1 ? x0 : x1;
	maxy = y0 > y1 ? y0 : y1;
	if (minx < 0) minx = 0;
	if (miny < 0) miny = 0;
	if (maxx >= img->width) maxx = img->width - 1;
	if (maxy >= img->height) maxy = img->height - 1;

	tmp = malloc(img->len);
	if (!tmp) {
		return;
	}
	memcpy(tmp, img->pixels, img->len);

	for (y = miny; y <= maxy; y++) {
		for (x = minx; x <= maxx; x++) {
			int nx;
			int ny;
			unsigned int rsum = 0;
			unsigned int gsum = 0;
			unsigned int bsum = 0;
			unsigned int count = 0;
			for (dy = -radius; dy <= radius; dy++) {
				for (dx = -radius; dx <= radius; dx++) {
					nx = x + dx;
					ny = y + dy;
					if (nx < minx || nx > maxx || ny < miny || ny > maxy) {
						continue;
					}
					{
						unsigned char *p = tmp + ((size_t)ny * (size_t)img->width + (size_t)nx) * 4;
						rsum += p[0];
						gsum += p[1];
						bsum += p[2];
						count++;
					}
				}
			}
			if (count > 0) {
				unsigned char *dst = img->pixels + ((size_t)y * (size_t)img->width + (size_t)x) * 4;
				dst[0] = (unsigned char)(rsum / count);
				dst[1] = (unsigned char)(gsum / count);
				dst[2] = (unsigned char)(bsum / count);
				dst[3] = 0xff;
			}
		}
	}

	free(tmp);
}

unsigned int
draw_sample_color(const struct image *img, int x, int y)
{
	const unsigned char *p;

	if (!in_bounds(img, x, y)) {
		return 0xffffff;
	}
	p = img->pixels + ((size_t)y * (size_t)img->width + (size_t)x) * 4;
	return ((unsigned int)p[0] << 16) | ((unsigned int)p[1] << 8) | (unsigned int)p[2];
}
