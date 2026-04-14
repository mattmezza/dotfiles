/* nod - suckless notification daemon
 * See LICENSE file for copyright and license details.
 */

#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xrandr.h>

#include <dbus/dbus.h>

#include "config.h"

#define LEN(x)       (sizeof(x) / sizeof((x)[0]))
#define MAX_NOTIFS   32
#define BUS_NAME     "org.freedesktop.Notifications"
#define OBJ_PATH     "/org/freedesktop/Notifications"
#define IFACE        "org.freedesktop.Notifications"

typedef struct {
	uint32_t id;
	int      active;
	char    *summary;
	char    *body;
	int      urgency;
	int      has_default_action;
	int      timeout_ms; /* 0 = persistent */
	struct timespec created;
	Window   win;
	int      x, y, w, h;
	char   **lines;
	int      nlines;
	int      summary_nlines;
} Notif;

static Display        *dpy;
static int             screen;
static Window          root;
static Visual         *visual;
static Colormap        cmap;
static int             depth;
static XftFont        *xfont;
static XftFont        *xfont_bold;
static XftColor        bg_xft[3], fg_xft[3], border_xft[3];
static Cursor          hand_cursor = 0;

static DBusConnection *dbus = NULL;

static Notif           notifs[MAX_NOTIFS];
static uint32_t        next_id = 1;
static volatile sig_atomic_t running = 1;

/* ---------- utils ---------- */

static void
die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(1);
}

static void *
ecalloc(size_t n, size_t s)
{
	void *p = calloc(n, s);
	if (!p) die("calloc: out of memory");
	return p;
}

static void *
erealloc(void *p, size_t s)
{
	void *r = realloc(p, s);
	if (!r) die("realloc: out of memory");
	return r;
}

static char *
estrdup(const char *s)
{
	char *r = strdup(s ? s : "");
	if (!r) die("strdup: out of memory");
	return r;
}

/* Strip HTML/pango markup. Decode basic entities. Return newly allocated. */
static char *
strip_markup(const char *in)
{
	if (!in) return estrdup("");
	size_t n = strlen(in);
	char *out = ecalloc(1, n + 1);
	size_t o = 0;
	for (size_t i = 0; i < n; ) {
		if (in[i] == '<') {
			while (i < n && in[i] != '>') i++;
			if (i < n) i++;
		} else if (in[i] == '&') {
			if (!strncmp(in + i, "&amp;", 5))       { out[o++] = '&';  i += 5; }
			else if (!strncmp(in + i, "&lt;", 4))   { out[o++] = '<';  i += 4; }
			else if (!strncmp(in + i, "&gt;", 4))   { out[o++] = '>';  i += 4; }
			else if (!strncmp(in + i, "&quot;", 6)) { out[o++] = '"';  i += 6; }
			else if (!strncmp(in + i, "&apos;", 6)) { out[o++] = '\''; i += 6; }
			else if (!strncmp(in + i, "&#", 2)) {
				/* numeric entity — skip to ; */
				size_t j = i + 2;
				while (j < n && in[j] != ';') j++;
				if (j < n) i = j + 1; else out[o++] = in[i++];
			} else {
				out[o++] = in[i++];
			}
		} else if (in[i] == '\t') {
			out[o++] = ' '; i++;
		} else if (in[i] == '\r') {
			i++;
		} else {
			out[o++] = in[i++];
		}
	}
	out[o] = 0;
	return out;
}

static int
text_width(XftFont *f, const char *s, int len)
{
	XGlyphInfo gi;
	XftTextExtentsUtf8(dpy, f, (FcChar8 *)s, len, &gi);
	return gi.xOff;
}

/* Greedy word-wrap. Respects '\n'. Returns array of newly-allocated strings. */
static char **
wrap_text(XftFont *f, const char *text, int maxw, int maxlines, int *nlines_out)
{
	char **lines = NULL;
	int cap = 0, n = 0;
	int truncated = 0;
	const char *p = text;

	while (*p) {
		if (maxlines > 0 && n >= maxlines) { truncated = 1; break; }

		while (*p == ' ') p++;
		if (!*p) break;
		if (*p == '\n') {
			if (n == cap) { cap = cap ? cap * 2 : 4; lines = erealloc(lines, cap * sizeof(char *)); }
			lines[n++] = estrdup("");
			p++;
			continue;
		}

		const char *eol = strchr(p, '\n');
		int remaining = eol ? (int)(eol - p) : (int)strlen(p);

		int fit = 0, last_space = -1, i;
		for (i = 1; i <= remaining; i++) {
			if (text_width(f, p, i) > maxw) break;
			if (p[i - 1] == ' ') last_space = i;
			fit = i;
		}

		int brk;
		if (fit == remaining) brk = fit;
		else brk = (last_space > 0) ? last_space : (fit > 0 ? fit : 1);

		char *line = ecalloc(1, brk + 1);
		memcpy(line, p, brk);
		while (brk > 0 && line[brk - 1] == ' ') line[--brk] = 0;

		if (n == cap) { cap = cap ? cap * 2 : 4; lines = erealloc(lines, cap * sizeof(char *)); }
		lines[n++] = line;

		p += brk;
		while (*p == ' ') p++;
		if (fit == remaining && *p == '\n') p++;
	}

	if (truncated && n > 0) {
		char *last = lines[n - 1];
		size_t ll = strlen(last);
		char *nl = ecalloc(1, ll + 4);
		memcpy(nl, last, ll);
		strcpy(nl + ll, "...");
		while (strlen(nl) > 3 && text_width(f, nl, strlen(nl)) > maxw) {
			size_t sl = strlen(nl);
			memmove(nl + sl - 4, nl + sl - 3, 4);
		}
		free(last);
		lines[n - 1] = nl;
	}

	*nlines_out = n;
	return lines;
}

/* ---------- monitor detection ---------- */

static void
get_focused_monitor(int *mx, int *my, int *mw, int *mh)
{
	int px = 0, py = 0, have_point = 0;

	Atom net_active = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop = NULL;

	if (XGetWindowProperty(dpy, root, net_active, 0, 1, False, XA_WINDOW,
	        &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success
	    && prop && nitems > 0) {
		Window aw = *(Window *)prop;
		if (aw) {
			XWindowAttributes wa;
			Window child;
			int rx, ry;
			if (XGetWindowAttributes(dpy, aw, &wa)
			    && XTranslateCoordinates(dpy, aw, root, 0, 0, &rx, &ry, &child)) {
				px = rx + wa.width / 2;
				py = ry + wa.height / 2;
				have_point = 1;
			}
		}
	}
	if (prop) XFree(prop);

	if (!have_point) {
		Window rret, cret;
		int rx, ry, wx, wy;
		unsigned int mask;
		if (XQueryPointer(dpy, root, &rret, &cret, &rx, &ry, &wx, &wy, &mask)) {
			px = rx; py = ry; have_point = 1;
		}
	}

	int nmon = 0;
	XRRMonitorInfo *mons = XRRGetMonitors(dpy, root, True, &nmon);
	if (mons && nmon > 0) {
		int chosen = -1;
		for (int i = 0; i < nmon; i++) {
			if (have_point &&
			    px >= mons[i].x && px < mons[i].x + mons[i].width &&
			    py >= mons[i].y && py < mons[i].y + mons[i].height) {
				chosen = i;
				break;
			}
		}
		if (chosen < 0) {
			for (int i = 0; i < nmon; i++) if (mons[i].primary) { chosen = i; break; }
		}
		if (chosen < 0) chosen = 0;
		*mx = mons[chosen].x;
		*my = mons[chosen].y;
		*mw = mons[chosen].width;
		*mh = mons[chosen].height;
		XRRFreeMonitors(mons);
	} else {
		*mx = 0; *my = 0;
		*mw = DisplayWidth(dpy, screen);
		*mh = DisplayHeight(dpy, screen);
	}
}

/* ---------- X init ---------- */

static XftColor
alloc_color(const char *name)
{
	XftColor c;
	if (!XftColorAllocName(dpy, visual, cmap, name, &c))
		die("cannot allocate color: %s", name);
	return c;
}

static int
x_error_handler(Display *d, XErrorEvent *e)
{
	(void)d; (void)e;
	/* Ignore X errors; they're typically on windows being torn down. */
	return 0;
}

static void
x_init(void)
{
	dpy = XOpenDisplay(NULL);
	if (!dpy) die("cannot open display");
	XSetErrorHandler(x_error_handler);
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	visual = DefaultVisual(dpy, screen);
	cmap = DefaultColormap(dpy, screen);
	depth = DefaultDepth(dpy, screen);

	xfont = XftFontOpenName(dpy, screen, font);
	if (!xfont) die("cannot load font: %s", font);

	/* Try to load a bold variant; fontconfig falls through to
	 * whatever matches closest, so a non-bold font still works. */
	size_t fn = strlen(font);
	char *bfn = ecalloc(1, fn + 32);
	snprintf(bfn, fn + 32, "%s:style=Bold", font);
	xfont_bold = XftFontOpenName(dpy, screen, bfn);
	free(bfn);
	if (!xfont_bold) xfont_bold = xfont;

	for (int i = 0; i < 3; i++) {
		bg_xft[i]     = alloc_color(bg_colors[i]);
		fg_xft[i]     = alloc_color(fg_colors[i]);
		border_xft[i] = alloc_color(border_colors[i]);
	}

	hand_cursor = XCreateFontCursor(dpy, XC_hand2);
}

/* ---------- notification table ---------- */

static int
find_free_slot(void)
{
	for (int i = 0; i < MAX_NOTIFS; i++)
		if (!notifs[i].active) return i;
	return -1;
}

static int
find_by_id(uint32_t id)
{
	for (int i = 0; i < MAX_NOTIFS; i++)
		if (notifs[i].active && notifs[i].id == id) return i;
	return -1;
}

static int
count_active(void)
{
	int c = 0;
	for (int i = 0; i < MAX_NOTIFS; i++) if (notifs[i].active) c++;
	return c;
}

static int
find_oldest(void)
{
	int oldest = -1;
	for (int i = 0; i < MAX_NOTIFS; i++) {
		if (!notifs[i].active) continue;
		if (oldest < 0 ||
		    notifs[i].created.tv_sec < notifs[oldest].created.tv_sec ||
		    (notifs[i].created.tv_sec == notifs[oldest].created.tv_sec &&
		     notifs[i].created.tv_nsec < notifs[oldest].created.tv_nsec))
			oldest = i;
	}
	return oldest;
}

static void
free_lines(Notif *n)
{
	if (n->lines) {
		for (int i = 0; i < n->nlines; i++) free(n->lines[i]);
		free(n->lines);
		n->lines = NULL;
	}
	n->nlines = 0;
}

static void
free_notif_content(Notif *n)
{
	free(n->summary); n->summary = NULL;
	free(n->body);    n->body = NULL;
	free_lines(n);
}

static void
layout_notif(Notif *n)
{
	free_lines(n);

	int inner_maxw = max_width - 2 * padding;
	if (inner_maxw < 20) inner_maxw = 20;

	int sn = 0, bn = 0;
	char **sl = wrap_text(xfont_bold,
	                      n->summary ? n->summary : "", inner_maxw, 1, &sn);
	char **bl = NULL;
	if (n->body && n->body[0])
		bl = wrap_text(xfont, n->body, inner_maxw, max_lines, &bn);

	int total = sn + bn;
	if (total == 0) total = 1;
	n->lines = ecalloc(total, sizeof(char *));
	int k = 0;
	for (int i = 0; i < sn; i++) n->lines[k++] = sl[i];
	for (int i = 0; i < bn; i++) n->lines[k++] = bl[i];
	if (k == 0) n->lines[k++] = estrdup("");
	n->nlines = k;
	n->summary_nlines = sn;
	free(sl);
	free(bl);

	int maxtw = 0;
	for (int i = 0; i < n->nlines; i++) {
		XftFont *f = (i < n->summary_nlines) ? xfont_bold : xfont;
		int tw = text_width(f, n->lines[i], strlen(n->lines[i]));
		if (tw > maxtw) maxtw = tw;
	}
	n->w = maxtw + 2 * padding;
	if (n->w > max_width) n->w = max_width;
	if (n->w < 2 * padding + 20) n->w = 2 * padding + 20;

	int lh_b = xfont_bold->ascent + xfont_bold->descent;
	int lh_r = xfont->ascent + xfont->descent;
	int body_n = n->nlines - n->summary_nlines;
	int gap = (n->summary_nlines > 0 && body_n > 0) ? title_gap : 0;
	n->h = n->summary_nlines * lh_b + gap + body_n * lh_r + 2 * padding;
}

static void
create_window(Notif *n)
{
	XSetWindowAttributes swa;
	swa.override_redirect = True;
	swa.background_pixel = bg_xft[n->urgency].pixel;
	swa.border_pixel     = border_xft[n->urgency].pixel;
	swa.event_mask       = ExposureMask | ButtonPressMask;

	n->win = XCreateWindow(dpy, root,
	    n->x, n->y, n->w, n->h, border_width,
	    depth, InputOutput, visual,
	    CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWEventMask,
	    &swa);

	XClassHint ch = { .res_name = "nod", .res_class = "nod" };
	XSetClassHint(dpy, n->win, &ch);

	if (n->has_default_action && hand_cursor)
		XDefineCursor(dpy, n->win, hand_cursor);
}

static void
draw_notif(Notif *n)
{
	if (!n->win) return;
	XftDraw *xd = XftDrawCreate(dpy, n->win, visual, cmap);
	XClearWindow(dpy, n->win);
	int lh_b = xfont_bold->ascent + xfont_bold->descent;
	int lh_r = xfont->ascent + xfont->descent;
	int body_n = n->nlines - n->summary_nlines;
	int gap = (n->summary_nlines > 0 && body_n > 0) ? title_gap : 0;
	int y = padding;
	for (int i = 0; i < n->nlines; i++) {
		if (i == n->summary_nlines && gap) y += gap;
		XftFont *f = (i < n->summary_nlines) ? xfont_bold : xfont;
		int line_lh = (i < n->summary_nlines) ? lh_b : lh_r;
		XftDrawStringUtf8(xd, &fg_xft[n->urgency], f,
		    padding, y + f->ascent,
		    (FcChar8 *)n->lines[i], strlen(n->lines[i]));
		y += line_lh;
	}
	XftDrawDestroy(xd);
}

static void
reposition_all(void)
{
	int mx, my, mw, mh;
	get_focused_monitor(&mx, &my, &mw, &mh);

	int idx[MAX_NOTIFS];
	int n = 0;
	for (int i = 0; i < MAX_NOTIFS; i++)
		if (notifs[i].active) idx[n++] = i;

	for (int i = 0; i < n; i++) {
		for (int j = i + 1; j < n; j++) {
			Notif *a = &notifs[idx[i]], *b = &notifs[idx[j]];
			if (a->created.tv_sec > b->created.tv_sec ||
			    (a->created.tv_sec == b->created.tv_sec &&
			     a->created.tv_nsec > b->created.tv_nsec)) {
				int t = idx[i]; idx[i] = idx[j]; idx[j] = t;
			}
		}
	}

	int top  = (position == TOP_LEFT || position == TOP_RIGHT);
	int left = (position == TOP_LEFT || position == BOTTOM_LEFT);

	int cursor = top ? (my + gap_y) : (my + mh - gap_y);

	for (int i = 0; i < n; i++) {
		Notif *ni = &notifs[idx[i]];
		int wx = left ? (mx + gap_x)
		              : (mx + mw - gap_x - ni->w - 2 * border_width);
		int wy;
		if (top) {
			wy = cursor;
			cursor += ni->h + 2 * border_width + stack_gap;
		} else {
			wy = cursor - ni->h - 2 * border_width;
			cursor = wy - stack_gap;
		}
		ni->x = wx;
		ni->y = wy;
		if (ni->win)
			XMoveWindow(dpy, ni->win, wx, wy);
	}
}

/* ---------- dbus signal emit ---------- */

static void
emit_closed(uint32_t id, uint32_t reason)
{
	if (!dbus) return;
	DBusMessage *m = dbus_message_new_signal(OBJ_PATH, IFACE, "NotificationClosed");
	if (!m) return;
	dbus_message_append_args(m,
	    DBUS_TYPE_UINT32, &id,
	    DBUS_TYPE_UINT32, &reason,
	    DBUS_TYPE_INVALID);
	dbus_connection_send(dbus, m, NULL);
	dbus_message_unref(m);
}

static void
emit_action(uint32_t id, const char *key)
{
	if (!dbus) return;
	DBusMessage *m = dbus_message_new_signal(OBJ_PATH, IFACE, "ActionInvoked");
	if (!m) return;
	dbus_message_append_args(m,
	    DBUS_TYPE_UINT32, &id,
	    DBUS_TYPE_STRING, &key,
	    DBUS_TYPE_INVALID);
	dbus_connection_send(dbus, m, NULL);
	dbus_message_unref(m);
}

static void
dismiss(int i, uint32_t reason)
{
	if (!notifs[i].active) return;
	uint32_t id = notifs[i].id;
	if (notifs[i].win) {
		XUnmapWindow(dpy, notifs[i].win);
		XDestroyWindow(dpy, notifs[i].win);
		notifs[i].win = 0;
	}
	free_notif_content(&notifs[i]);
	notifs[i].active = 0;
	emit_closed(id, reason);
	reposition_all();
	XFlush(dpy);
}

/* ---------- Notify handler ---------- */

static uint32_t
handle_notify(DBusMessage *msg)
{
	DBusMessageIter args;
	if (!dbus_message_iter_init(msg, &args)) return 0;

	const char *app_name = "", *app_icon = "", *summary = "", *body = "";
	uint32_t replaces_id = 0;
	int32_t  expire_timeout = -1;
	int      urgency = 1;
	int      has_default = 0;

	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING) return 0;
	dbus_message_iter_get_basic(&args, &app_name);
	dbus_message_iter_next(&args);

	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_UINT32) return 0;
	dbus_message_iter_get_basic(&args, &replaces_id);
	dbus_message_iter_next(&args);

	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING) return 0;
	dbus_message_iter_get_basic(&args, &app_icon);
	dbus_message_iter_next(&args);

	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING) return 0;
	dbus_message_iter_get_basic(&args, &summary);
	dbus_message_iter_next(&args);

	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING) return 0;
	dbus_message_iter_get_basic(&args, &body);
	dbus_message_iter_next(&args);

	/* actions: as — alternating key, label */
	if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_ARRAY) {
		DBusMessageIter ai;
		dbus_message_iter_recurse(&args, &ai);
		int k = 0;
		while (dbus_message_iter_get_arg_type(&ai) == DBUS_TYPE_STRING) {
			if (k % 2 == 0) {
				const char *key;
				dbus_message_iter_get_basic(&ai, &key);
				if (key && !strcmp(key, "default")) has_default = 1;
			}
			k++;
			dbus_message_iter_next(&ai);
		}
	}
	dbus_message_iter_next(&args);

	/* hints: a{sv} */
	if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_ARRAY) {
		DBusMessageIter hi;
		dbus_message_iter_recurse(&args, &hi);
		while (dbus_message_iter_get_arg_type(&hi) == DBUS_TYPE_DICT_ENTRY) {
			DBusMessageIter de;
			dbus_message_iter_recurse(&hi, &de);
			const char *key = NULL;
			if (dbus_message_iter_get_arg_type(&de) == DBUS_TYPE_STRING)
				dbus_message_iter_get_basic(&de, &key);
			dbus_message_iter_next(&de);
			if (key && !strcmp(key, "urgency")
			    && dbus_message_iter_get_arg_type(&de) == DBUS_TYPE_VARIANT) {
				DBusMessageIter var;
				dbus_message_iter_recurse(&de, &var);
				if (dbus_message_iter_get_arg_type(&var) == DBUS_TYPE_BYTE) {
					uint8_t u = 0;
					dbus_message_iter_get_basic(&var, &u);
					if (u <= 2) urgency = u;
				}
			}
			dbus_message_iter_next(&hi);
		}
	}
	dbus_message_iter_next(&args);

	if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_INT32)
		dbus_message_iter_get_basic(&args, &expire_timeout);

	(void)app_name; (void)app_icon;

	/* locate / allocate slot */
	int slot = -1;
	int is_replace = 0;
	if (replaces_id != 0) {
		slot = find_by_id(replaces_id);
		if (slot >= 0) is_replace = 1;
	}
	if (slot < 0) {
		if (count_active() >= max_visible) {
			int o = find_oldest();
			if (o >= 0) dismiss(o, 2);
		}
		slot = find_free_slot();
		if (slot < 0) {
			int o = find_oldest();
			if (o >= 0) { dismiss(o, 2); slot = find_free_slot(); }
		}
		if (slot < 0) return 0;
	}

	Notif *n = &notifs[slot];
	uint32_t id;
	if (is_replace) {
		id = n->id;
		free_notif_content(n);
	} else {
		id = next_id++;
		if (next_id == 0) next_id = 1;
		memset(n, 0, sizeof(*n));
		n->id = id;
	}

	n->summary = strip_markup(summary);
	n->body    = strip_markup(body);
	n->urgency = urgency;
	n->has_default_action = has_default;

	int tm;
	if (expire_timeout > 0) tm = expire_timeout;
	else                    tm = timeouts[urgency] * 1000;
	n->timeout_ms = tm;

	clock_gettime(CLOCK_MONOTONIC, &n->created);
	n->active = 1;

	layout_notif(n);

	if (is_replace && n->win) {
		XSetWindowBackground(dpy, n->win, bg_xft[n->urgency].pixel);
		XSetWindowBorder(dpy, n->win, border_xft[n->urgency].pixel);
		XResizeWindow(dpy, n->win, n->w, n->h);
		if (n->has_default_action && hand_cursor)
			XDefineCursor(dpy, n->win, hand_cursor);
		else
			XUndefineCursor(dpy, n->win);
	} else {
		create_window(n);
		XMapRaised(dpy, n->win);
	}

	reposition_all();
	draw_notif(n);
	XFlush(dpy);
	return id;
}

/* ---------- introspection + dispatch ---------- */

static const char *introspect_xml =
	"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
	" \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
	"<node>\n"
	"  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
	"    <method name=\"Introspect\">\n"
	"      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
	"    </method>\n"
	"  </interface>\n"
	"  <interface name=\"org.freedesktop.Notifications\">\n"
	"    <method name=\"GetCapabilities\">\n"
	"      <arg direction=\"out\" name=\"caps\" type=\"as\"/>\n"
	"    </method>\n"
	"    <method name=\"Notify\">\n"
	"      <arg direction=\"in\"  name=\"app_name\"       type=\"s\"/>\n"
	"      <arg direction=\"in\"  name=\"replaces_id\"    type=\"u\"/>\n"
	"      <arg direction=\"in\"  name=\"app_icon\"       type=\"s\"/>\n"
	"      <arg direction=\"in\"  name=\"summary\"        type=\"s\"/>\n"
	"      <arg direction=\"in\"  name=\"body\"           type=\"s\"/>\n"
	"      <arg direction=\"in\"  name=\"actions\"        type=\"as\"/>\n"
	"      <arg direction=\"in\"  name=\"hints\"          type=\"a{sv}\"/>\n"
	"      <arg direction=\"in\"  name=\"expire_timeout\" type=\"i\"/>\n"
	"      <arg direction=\"out\" name=\"id\"             type=\"u\"/>\n"
	"    </method>\n"
	"    <method name=\"CloseNotification\">\n"
	"      <arg direction=\"in\" name=\"id\" type=\"u\"/>\n"
	"    </method>\n"
	"    <method name=\"GetServerInformation\">\n"
	"      <arg direction=\"out\" name=\"name\"         type=\"s\"/>\n"
	"      <arg direction=\"out\" name=\"vendor\"       type=\"s\"/>\n"
	"      <arg direction=\"out\" name=\"version\"      type=\"s\"/>\n"
	"      <arg direction=\"out\" name=\"spec_version\" type=\"s\"/>\n"
	"    </method>\n"
	"    <signal name=\"NotificationClosed\">\n"
	"      <arg name=\"id\"     type=\"u\"/>\n"
	"      <arg name=\"reason\" type=\"u\"/>\n"
	"    </signal>\n"
	"    <signal name=\"ActionInvoked\">\n"
	"      <arg name=\"id\"         type=\"u\"/>\n"
	"      <arg name=\"action_key\" type=\"s\"/>\n"
	"    </signal>\n"
	"  </interface>\n"
	"</node>\n";

static DBusHandlerResult
dbus_msg_handler(DBusConnection *c, DBusMessage *msg, void *ud)
{
	(void)ud;
	const char *iface  = dbus_message_get_interface(msg);
	const char *member = dbus_message_get_member(msg);
	DBusMessage *reply = NULL;

	if (!iface || !member)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (!strcmp(iface, "org.freedesktop.DBus.Introspectable")
	    && !strcmp(member, "Introspect")) {
		reply = dbus_message_new_method_return(msg);
		if (reply) dbus_message_append_args(reply,
		    DBUS_TYPE_STRING, &introspect_xml, DBUS_TYPE_INVALID);
	} else if (!strcmp(iface, IFACE)) {
		if (!strcmp(member, "GetCapabilities")) {
			reply = dbus_message_new_method_return(msg);
			if (reply) {
				DBusMessageIter it, sub;
				dbus_message_iter_init_append(reply, &it);
				dbus_message_iter_open_container(&it, DBUS_TYPE_ARRAY, "s", &sub);
				const char *c1 = "body", *c2 = "actions";
				dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &c1);
				dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &c2);
				dbus_message_iter_close_container(&it, &sub);
			}
		} else if (!strcmp(member, "GetServerInformation")) {
			reply = dbus_message_new_method_return(msg);
			if (reply) {
				const char *name = "nod", *vendor = "nod";
				const char *ver  = VERSION, *spec = "1.2";
				dbus_message_append_args(reply,
				    DBUS_TYPE_STRING, &name,
				    DBUS_TYPE_STRING, &vendor,
				    DBUS_TYPE_STRING, &ver,
				    DBUS_TYPE_STRING, &spec,
				    DBUS_TYPE_INVALID);
			}
		} else if (!strcmp(member, "Notify")) {
			uint32_t id = handle_notify(msg);
			reply = dbus_message_new_method_return(msg);
			if (reply) dbus_message_append_args(reply,
			    DBUS_TYPE_UINT32, &id, DBUS_TYPE_INVALID);
		} else if (!strcmp(member, "CloseNotification")) {
			uint32_t id = 0;
			dbus_message_get_args(msg, NULL,
			    DBUS_TYPE_UINT32, &id, DBUS_TYPE_INVALID);
			int i = find_by_id(id);
			if (i >= 0) dismiss(i, 3);
			reply = dbus_message_new_method_return(msg);
		} else {
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}
	} else {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (reply) {
		dbus_connection_send(c, reply, NULL);
		dbus_message_unref(reply);
	}
	return DBUS_HANDLER_RESULT_HANDLED;
}

static void
dbus_init(void)
{
	DBusError err;
	dbus_error_init(&err);
	dbus = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if (dbus_error_is_set(&err)) die("dbus: %s", err.message);
	if (!dbus) die("cannot connect to session bus");

	int r = dbus_bus_request_name(dbus, BUS_NAME,
	    DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
	if (dbus_error_is_set(&err)) die("dbus request_name: %s", err.message);
	if (r != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
		die("another notification daemon owns %s", BUS_NAME);

	static const DBusObjectPathVTable vt = {
		.message_function = dbus_msg_handler,
	};
	if (!dbus_connection_register_object_path(dbus, OBJ_PATH, &vt, NULL))
		die("dbus register object path failed");
}

/* ---------- X events ---------- */

static void
handle_x_event(XEvent *ev)
{
	switch (ev->type) {
	case Expose: {
		for (int i = 0; i < MAX_NOTIFS; i++)
			if (notifs[i].active && notifs[i].win == ev->xexpose.window) {
				draw_notif(&notifs[i]);
				break;
			}
		break;
	}
	case ButtonPress: {
		int i;
		for (i = 0; i < MAX_NOTIFS; i++)
			if (notifs[i].active && notifs[i].win == ev->xbutton.window) break;
		if (i == MAX_NOTIFS) break;
		if (ev->xbutton.button == Button1) {
			if (notifs[i].has_default_action)
				emit_action(notifs[i].id, "default");
			dismiss(i, 2);
		} else if (ev->xbutton.button == Button3) {
			dismiss(i, 2);
		}
		break;
	}
	default:
		break;
	}
}

/* ---------- main loop ---------- */

static void
on_signal(int sig) { (void)sig; running = 0; }

static long
ms_remaining(Notif *n, struct timespec *now)
{
	if (n->timeout_ms <= 0) return -1;
	long elapsed = (now->tv_sec  - n->created.tv_sec)  * 1000L
	             + (now->tv_nsec - n->created.tv_nsec) / 1000000L;
	long rem = n->timeout_ms - elapsed;
	return rem < 0 ? 0 : rem;
}

int
main(int argc, char *argv[])
{
	if (argc > 1) {
		if (!strcmp(argv[1], "-v")) {
			printf("nod %s\n", VERSION);
			return 0;
		}
		fprintf(stderr, "usage: nod [-v]\n");
		return 1;
	}

	struct sigaction sa = { 0 };
	sa.sa_handler = on_signal;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);
	signal(SIGPIPE, SIG_IGN);

	x_init();
	dbus_init();

	int dfd = -1;
	if (!dbus_connection_get_unix_fd(dbus, &dfd))
		die("cannot get dbus fd");
	int xfd = ConnectionNumber(dpy);

	while (running) {
		while (XPending(dpy)) {
			XEvent ev;
			XNextEvent(dpy, &ev);
			handle_x_event(&ev);
		}

		while (dbus_connection_get_dispatch_status(dbus) == DBUS_DISPATCH_DATA_REMAINS)
			dbus_connection_dispatch(dbus);

		dbus_connection_flush(dbus);

		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		long next_ms = -1;
		for (int i = 0; i < MAX_NOTIFS; i++) {
			if (!notifs[i].active) continue;
			long r = ms_remaining(&notifs[i], &now);
			if (r < 0) continue;
			if (next_ms < 0 || r < next_ms) next_ms = r;
		}
		int timeout;
		if (next_ms < 0) timeout = -1;
		else if (next_ms > INT_MAX) timeout = INT_MAX;
		else timeout = (int)next_ms;

		struct pollfd pfd[2];
		pfd[0].fd = xfd; pfd[0].events = POLLIN; pfd[0].revents = 0;
		pfd[1].fd = dfd; pfd[1].events = POLLIN; pfd[1].revents = 0;

		int pr = poll(pfd, 2, timeout);
		if (pr < 0 && errno != EINTR) break;

		dbus_connection_read_write(dbus, 0);
		while (dbus_connection_get_dispatch_status(dbus) == DBUS_DISPATCH_DATA_REMAINS)
			dbus_connection_dispatch(dbus);

		clock_gettime(CLOCK_MONOTONIC, &now);
		for (int i = 0; i < MAX_NOTIFS; i++) {
			if (!notifs[i].active) continue;
			if (notifs[i].timeout_ms <= 0) continue;
			if (ms_remaining(&notifs[i], &now) == 0)
				dismiss(i, 1);
		}
	}

	for (int i = 0; i < MAX_NOTIFS; i++)
		if (notifs[i].active) dismiss(i, 3);

	if (dbus) {
		dbus_connection_flush(dbus);
		dbus_bus_release_name(dbus, BUS_NAME, NULL);
		dbus_connection_unref(dbus);
	}
	if (hand_cursor) XFreeCursor(dpy, hand_cursor);
	if (xfont_bold && xfont_bold != xfont) XftFontClose(dpy, xfont_bold);
	if (xfont) XftFontClose(dpy, xfont);
	if (dpy)   XCloseDisplay(dpy);
	return 0;
}
