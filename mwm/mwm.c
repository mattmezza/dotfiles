/* mwm - matteo's window manager
 *
 * A skimmed-down, dwm-flavoured tiling window manager. Design highlights:
 *   - one global pool of tags (max MAX_TAGS); each tag is owned by exactly one
 *     monitor (window in tag, tag on monitor: window <= tag <= monitor);
 *   - two tiling layouts (cols "|||", master+stack "||="), plus floating;
 *   - configurable gaps, optional rounded corners (X Shape, compositor-free);
 *   - a translucent three-pill bar (left tags+layout, center title, right
 *     status read from the root window name a la dwm/slstatus) + XEmbed systray;
 *   - NO built-in keybindings: everything is driven through the `mwmc` client
 *     over a pure-X11 ClientMessage IPC channel (see ipc.h).
 *
 * The code intentionally mirrors dwm's structure where it can, so dwm patches
 * and idioms map over cleanly. See LICENSE (MIT/X) for copyright details.
 */
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>

#include "drw.h"
#include "util.h"
#include "ipc.h"

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->mx+(m)->mw) - MAX((x),(m)->mx)) \
                               * MAX(0, MIN((y)+(h),(m)->my+(m)->mh) - MAX((y),(m)->my)))
#define ISVISIBLE(C)            (isvisible(C))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define OPAQUE                  0xffU
#define MAX_TAGS                100

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel, SchemeUrg, SchemeTitle, SchemeStatus, SchemeBar, SchemeLast }; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
       NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation, NetSystemTrayOrientationHorz,
       NetSystemTrayVisual, NetWMWindowType, NetWMWindowTypeDock, NetWMWindowTypeDialog,
       NetWMFullscreen, NetActiveWindow, NetWMStateDemandsAttention, NetClientList, NetLast };
enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { MwmMsg, MwmCmd, MwmReply, MwmLast }; /* IPC atoms */

typedef struct Monitor Monitor;
typedef struct Client Client;

struct Client {
	char name[256];
	char class[64], instance[64];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
	int bw, oldbw;
	int tag;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen, issticky;
	Client *next;   /* attach/tile order (global list) */
	Client *snext;  /* focus stack order (global list)  */
	Window win;
};

struct Monitor {
	char ltsymbol[16];
	int num;
	int mx, my, mw, mh;   /* monitor geometry */
	int wx, wy, ww, wh;   /* window/work area  */
	int by;               /* bar y position    */
	int seltag;           /* currently shown tag (-1 = none) */
	Client *sel;          /* last focused client on this monitor */
	Window barwin;
	Monitor *next;
};

typedef struct {
	int layout;     /* 0 cols, 1 stack, 2 float */
	int nmaster;
	float mfact;
	int mon;        /* owning monitor index, -1 = unassigned */
} Tag;

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	const char *tag;      /* tag name or 1-based number; NULL/"" = keep current */
	const char *wintype;  /* full _NET_WM_WINDOW_TYPE_* atom name; NULL = match any */
	int isfloating;
} Rule;

typedef struct {
	int tag;        /* tag index            */
	int mon;        /* default owner monitor index when that monitor appears */
} TagRule;

typedef struct Systray {
	Window win;
	Client *icons;
} Systray;

/* function declarations */
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachafter(Client *c, Client *after);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void clearround(Window w);
static Monitor *clientmon(const Client *c);
static void clientmessage(XEvent *e);
static void cols(Monitor *m, int t);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(int num);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static void dispatchcmd(int argc, char **argv, char *reply, size_t rsz);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusclientbyclass(const char *cls);
static void focusin(XEvent *e);
static void focusmon(const char *arg);
static void focusstack(int dir);
static Atom getatomprop(Client *c, Atom prop);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static unsigned int getsystraywidth(void);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void handleipc(Window requestor);
static void incnmaster(const char *arg);
static int isvisible(const Client *c);
static void killfocused(void);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void motionnotify(XEvent *e);
static void movemouse(void);
static void resizemouse(void);
static void focuslast(void);
static void movewinmon(int dir);
static void winmove(int dx, int dy);
static void winresize(int dw, int dh);
static void wincenter(void);
static void movetagtomon(const char *arg);
static void movewintotag(const char *arg);
static Monitor *montag(int t);
static Client *nexttiled(Client *c, Monitor *m, int t);
static int occupied(int t);
static void propertynotify(XEvent *e);
static Monitor *recttomon(int x, int y, int w, int h);
static void removesystrayicon(Client *i);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizerequest(XEvent *e);
static void restack(Monitor *m);
static void maskroundrect(Pixmap mask, GC g, int x, int y, int w, int h, int rad);
static void roundwin(Window win, int w, int h, int rad, int bw);
static void shapebar(Monitor *m, int rects[][2], int n);
static void run(void);
static void scan(void);
static int sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4);
static void sendmon(Client *c, int tag);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setgap(const char *arg);
static void setlayout(const char *arg);
static void setmfact(const char *arg);
static int settag(const char *arg, int cur);
static void setup(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void sigchld(int unused);
static void spawnreply(Window w, const char *s);
static int tagcount(void);
static int taglabel(int t, char *buf, size_t n);
static void tile(Monitor *m, int t);
static void togglebar(void);
static void toggletraybar(void);
static void togglefloating(void);
static void togglefullscreen(void);
static void togglesticky(void);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(int initial);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatesystray(void);
static void updatesystrayicongeom(Client *i, int w, int h);
static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static void viewtag(const char *arg);
static void warptoclient(Client *c);
static void warptomon(Monitor *m);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static Client *wintosystrayicon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(void);

/* XEmbed / systray protocol constants */
#define SYSTEM_TRAY_REQUEST_DOCK    0
#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define XEMBED_EMBEDDED_NOTIFY      0
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_FOCUS_IN             4
#define XEMBED_MODALITY_ON         10
#define XEMBED_MAPPED          (1 << 0)
#define VERSION_MAJOR               0
#define VERSION_MINOR               0
#define XEMBED_EMBEDDED_VERSION ((VERSION_MAJOR << 16) | VERSION_MINOR)

/* variables */
static const char broken[] = "broken";
static char stext[512];
static int screen;
static int sw, sh;              /* X display screen geometry width, height */
static int bh;                  /* bar height */
static int lrpad;               /* sum of left and right padding for text */
static int gappx_run;           /* runtime-adjustable gap */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[ResizeRequest] = resizerequest,
	[UnmapNotify] = unmapnotify,
};
static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast], mwmatom[MwmLast], xa_utf8;
static int running = 1;
static int shapeext = 0, shapeevbase = 0;
static Cur *cursor[CurLast];
static Clr **scheme;            /* scheme[SchemeLast][3] */
static Clr *fillcol;            /* per-scheme pill background (alpha kept) */
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Client *clients = NULL;  /* global attach/tile order */
static Client *fstack = NULL;   /* global focus stack */
static Window root, wmcheckwin;
static Systray *systray = NULL;
static Tag tagstate[MAX_TAGS];

/* ARGB visual */
static Visual *visual;
static int depth;
static Colormap cmap;

/* Xresources styling: config.h maps resource names to addresses of the
 * (non-const) style variables below; load_xresources() overrides them at
 * startup from the X resource database (xrdb / ~/.Xresources). */
typedef enum { STRING_RES, INTEGER_RES, FLOAT_RES } ResType;
typedef struct {
	const char *name;
	ResType type;
	void *dst;
} ResourcePref;
static void resource_load(XrmDatabase db, const char *name, ResType type, void *dst);
static void load_xresources(void);
static const char *reloadstyle(void);

/* configuration, allows nested code to access above variables */
#include "config.h"

/* compile-time check that the number of named tags fits */
struct NumTags { char limitexceeded[LENGTH(tags) > MAX_TAGS ? -1 : 1]; };

/* ---- small helpers --------------------------------------------------- */

int
tagcount(void)
{
	return LENGTH(tags);
}

Monitor *
montag(int t)
{
	Monitor *m;
	if (t < 0 || t >= MAX_TAGS)
		return NULL;
	for (m = mons; m; m = m->next)
		if (m->num == tagstate[t].mon)
			return m;
	return NULL;
}

Monitor *
clientmon(const Client *c)
{
	return c ? montag(c->tag) : NULL;
}

int
isvisible(const Client *c)
{
	Monitor *o = montag(c->tag);
	return o && (o->seltag == c->tag || c->issticky);
}

int
occupied(int t)
{
	Client *c;
	for (c = clients; c; c = c->next)
		if (c->tag == t)
			return 1;
	return 0;
}

/* writes a printable label for tag t into buf, returns strlen */
int
taglabel(int t, char *buf, size_t n)
{
	if (t >= 0 && t < (int)LENGTH(tags))
		snprintf(buf, n, "%s", tags[t]);
	else
		snprintf(buf, n, "%d", t + 1);
	return strlen(buf);
}

/* next tiled client in `clients` order on monitor m showing tag t */
Client *
nexttiled(Client *c, Monitor *m, int t)
{
	for (; c && (c->tag != t || c->isfloating || c->isfullscreen || !ISVISIBLE(c)); c = c->next)
		;
	return c;
}

/* ---- rules ----------------------------------------------------------- */

/* case-insensitive strstr: rule class/instance/title matches ignore
 * capitalisation, so a rule "pinentry" matches WM_CLASS res_class
 * "Pinentry-gtk" (and "Pavucontrol", "Arandr", etc. likewise). Portable
 * (no strcasestr/_GNU_SOURCE), uses only C99 tolower. */
static char *
ci_strstr(const char *hay, const char *needle)
{
	size_t i, nl = strlen(needle);

	if (!nl)
		return (char *)hay;
	for (; *hay; hay++) {
		for (i = 0; needle[i] && hay[i]; i++)
			if (tolower((unsigned char)hay[i]) != tolower((unsigned char)needle[i]))
				break;
		if (!needle[i])
			return (char *)hay;
	}
	return NULL;
}

void
applyrules(Client *c)
{
	const char *class, *instance;
	unsigned int i;
	const Rule *r;
	Atom wt;
	XClassHint ch = { NULL, NULL };

	c->isfloating = 0;
	c->tag = -1;
	wt = getatomprop(c, netatom[NetWMWindowType]);
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;
	strncpy(c->class, class, sizeof(c->class) - 1);
	strncpy(c->instance, instance, sizeof(c->instance) - 1);

	for (i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if ((!r->title || ci_strstr(c->name, r->title))
		&& (!r->class || ci_strstr(class, r->class))
		&& (!r->instance || ci_strstr(instance, r->instance))
		&& (!r->wintype || (wt != None && wt == XInternAtom(dpy, r->wintype, False)))) {
			c->isfloating = r->isfloating;
			if (r->tag && r->tag[0]) {
				int t = settag(r->tag, -1);
				if (t >= 0)
					c->tag = t;
			}
		}
	}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);

	if (c->tag < 0) {
		/* default to the focused monitor's tag; if the monitor owns/shows no
		 * tag yet, claim the lowest UNOWNED tag for it (never steal a tag that
		 * already belongs to another monitor) */
		if (selmon->seltag < 0) {
			int t, nt = -1;
			for (t = 0; t < MAX_TAGS; t++)
				if (tagstate[t].mon < 0) { nt = t; break; }
			if (nt < 0)
				nt = 0;
			tagstate[nt].mon = selmon->num;
			selmon->seltag = nt;
		}
		c->tag = selmon->seltag;
	}
	/* if the target tag has no owner yet, give it to the focused monitor */
	if (tagstate[c->tag].mon < 0) {
		tagstate[c->tag].mon = selmon->num;
		if (montag(c->tag) && montag(c->tag)->seltag < 0)
			montag(c->tag)->seltag = c->tag;
	}
}

/* ---- geometry / size hints ------------------------------------------ */

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = clientmon(c);

	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw) *x = sw - WIDTH(c);
		if (*y > sh) *y = sh - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0) *x = 0;
		if (*y + *h + 2 * c->bw < 0) *y = 0;
	} else if (m) {
		if (*x >= m->wx + m->ww) *x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh) *y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * c->bw <= m->wx) *x = m->wx;
		if (*y + *h + 2 * c->bw <= m->wy) *y = m->wy;
	}
	if (*h < bh) *h = bh;
	if (*w < bh) *w = bh;
	if (resizehints || c->isfloating) {
		if (!c->hintsvalid)
			updatesizehints(c);
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) {
			*w -= c->basew;
			*h -= c->baseh;
		}
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h) *w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w) *h = *w * c->mina + 0.5;
		}
		if (baseismin) {
			*w -= c->basew;
			*h -= c->baseh;
		}
		if (c->incw) *w -= *w % c->incw;
		if (c->inch) *h -= *h % c->inch;
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw) *w = MIN(*w, c->maxw);
		if (c->maxh) *h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

/* ---- arrange / layouts ----------------------------------------------- */

void
arrange(Monitor *m)
{
	showhide(fstack);
	if (m) {
		arrangemon(m);
		restack(m);
	} else {
		for (m = mons; m; m = m->next)
			arrangemon(m);
		for (m = mons; m; m = m->next)
			restack(m);
	}
}

void
arrangemon(Monitor *m)
{
	int t = m->seltag;

	if (t < 0) {
		strncpy(m->ltsymbol, "", sizeof(m->ltsymbol));
		return;
	}
	strncpy(m->ltsymbol, layoutsym[tagstate[t].layout], sizeof(m->ltsymbol) - 1);
	switch (tagstate[t].layout) {
	case 0: cols(m, t); break;
	case 1: tile(m, t); break;
	default: break; /* floating: nothing to do */
	}
}

void
cols(Monitor *m, int t)
{
	Client *c, **arr;
	unsigned int i, n;
	int og = gappx_run, ig = gappx_run;
	int avail, base, extra, x, wcol, bw;

	for (n = 0, c = clients; c; c = c->next)
		if (c->tag == t && !c->isfloating && !c->isfullscreen && ISVISIBLE(c))
			n++;
	if (n == 0)
		return;
	arr = ecalloc(n, sizeof(Client *));
	for (i = 0, c = clients; c; c = c->next)
		if (c->tag == t && !c->isfloating && !c->isfullscreen && ISVISIBLE(c))
			arr[i++] = c;

	avail = m->ww - 2 * og - (int)(n - 1) * ig;
	if (avail < (int)n)
		avail = n;
	base = avail / n;
	extra = avail % n;
	x = m->wx + og;
	for (i = 0; i < n; i++) {
		bw = arr[i]->bw;
		wcol = base + ((int)i < extra ? 1 : 0);
		resize(arr[i], x, m->wy + og, wcol - 2 * bw, m->wh - 2 * og - 2 * bw, 0);
		x += wcol + ig;
	}
	free(arr);
}

void
tile(Monitor *m, int t)
{
	Client *c, **arr;
	unsigned int i, n, nmm, ns;
	int og = gappx_run, ig = gappx_run;
	int mw, sx, sw, availh, base, extra, y, hh, bw;

	for (n = 0, c = clients; c; c = c->next)
		if (c->tag == t && !c->isfloating && !c->isfullscreen && ISVISIBLE(c))
			n++;
	if (n == 0)
		return;
	arr = ecalloc(n, sizeof(Client *));
	for (i = 0, c = clients; c; c = c->next)
		if (c->tag == t && !c->isfloating && !c->isfullscreen && ISVISIBLE(c))
			arr[i++] = c;

	nmm = tagstate[t].nmaster;
	if (nmm > n)
		nmm = n;
	ns = n - nmm;

	if (nmm == 0)
		mw = 0;
	else if (ns > 0)
		mw = (m->ww - 2 * og - ig) * tagstate[t].mfact;
	else
		mw = m->ww - 2 * og;

	/* master column(s) */
	if (nmm > 0) {
		availh = m->wh - 2 * og - (int)(nmm - 1) * ig;
		if (availh < (int)nmm) availh = nmm;
		base = availh / nmm;
		extra = availh % nmm;
		y = m->wy + og;
		for (i = 0; i < nmm; i++) {
			bw = arr[i]->bw;
			hh = base + ((int)i < extra ? 1 : 0);
			resize(arr[i], m->wx + og, y, mw - 2 * bw, hh - 2 * bw, 0);
			y += hh + ig;
		}
	}
	/* stack column */
	if (ns > 0) {
		sx = m->wx + og + (nmm ? mw + ig : 0);
		sw = m->ww - 2 * og - (nmm ? mw + ig : 0);
		availh = m->wh - 2 * og - (int)(ns - 1) * ig;
		if (availh < (int)ns) availh = ns;
		base = availh / ns;
		extra = availh % ns;
		y = m->wy + og;
		for (i = 0; i < ns; i++) {
			bw = arr[nmm + i]->bw;
			hh = base + ((int)i < extra ? 1 : 0);
			resize(arr[nmm + i], sx, y, sw - 2 * bw, hh - 2 * bw, 0);
			y += hh + ig;
		}
	}
	free(arr);
}

/* ---- list management ------------------------------------------------- */

void
attach(Client *c)
{
	c->next = clients;
	clients = c;
}

void
attachafter(Client *c, Client *after)
{
	if (!after) {
		attach(c);
		return;
	}
	c->next = after->next;
	after->next = c;
}

void
attachstack(Client *c)
{
	c->snext = fstack;
	fstack = c;
}

void
detach(Client *c)
{
	Client **tc;
	for (tc = &clients; *tc && *tc != c; tc = &(*tc)->next)
		;
	*tc = c->next;
}

void
detachstack(Client *c)
{
	Client **tc;
	for (tc = &fstack; *tc && *tc != c; tc = &(*tc)->snext)
		;
	*tc = c->snext;

	if (c == c->snext) /* paranoia */
		c->snext = NULL;
	{
		Monitor *m;
		for (m = mons; m; m = m->next)
			if (m->sel == c)
				m->sel = NULL;
	}
}

/* ---- focus ----------------------------------------------------------- */

void
focus(Client *c)
{
	if (!c || !ISVISIBLE(c))
		for (c = fstack; c && (!ISVISIBLE(c) || clientmon(c) != selmon); c = c->snext)
			;
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (clientmon(c) && clientmon(c) != selmon)
			selmon = clientmon(c);
		if (c->isurgent)
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);
		/* match the focused border to the selected-workspace pill colour
		 * (SchemeSel background / "selbg") rather than a separate "selborder".
		 * Use the un-premultiplied .color, since .pixel has RGB premultiplied
		 * by selbgalpha (which would darken/hide the border). */
		XSetWindowBorder(dpy, c->win,
		                 0xff000000UL /* opaque: ARGB client windows hide an alpha-0 border */
		               | ((unsigned long)(scheme[SchemeSel][ColBg].color.red   >> 8) << 16)
		               | ((unsigned long)(scheme[SchemeSel][ColBg].color.green >> 8) <<  8)
		               |  (unsigned long)(scheme[SchemeSel][ColBg].color.blue  >> 8));
		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
	drawbars();
}

void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

void
setfocus(Client *c)
{
	if (!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow], XA_WINDOW, 32,
		                PropModeReplace, (unsigned char *) &(c->win), 1);
	}
	sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
}

void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(c, 0);
	XSetWindowBorder(dpy, c->win, (scheme[SchemeNorm][ColBorder].pixel & 0x00ffffff) | 0xff000000UL);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

void
focusstack(int dir)
{
	Client *c = NULL, *i;

	if (!selmon->sel || selmon->sel->isfullscreen)
		return;
	if (dir > 0) {
		for (c = selmon->sel->next; c && !(ISVISIBLE(c) && clientmon(c) == selmon); c = c->next)
			;
		if (!c)
			for (c = clients; c && !(ISVISIBLE(c) && clientmon(c) == selmon); c = c->next)
				;
	} else {
		for (i = clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i) && clientmon(i) == selmon)
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i) && clientmon(i) == selmon)
					c = i;
	}
	if (c) {
		focus(c);
		restack(selmon);
		warptoclient(c);
	}
}

void
focusclientbyclass(const char *cls)
{
	Client *c;
	for (c = clients; c; c = c->next)
		if (strstr(c->class, cls)) {
			Monitor *m = montag(c->tag);
			if (m) {
				m->seltag = c->tag;
				selmon = m;
			}
			arrange(NULL);
			focus(c);
			restack(selmon);
			warptoclient(c);
			return;
		}
}

void
focusmon(const char *arg)
{
	Monitor *m;

	if (!mons->next)
		return;
	if (!strcmp(arg, "next"))
		m = dirtomon(+1);
	else if (!strcmp(arg, "prev"))
		m = dirtomon(-1);
	else {
		int n = atoi(arg);
		for (m = mons; m && m->num != n; m = m->next)
			;
		if (!m)
			return;
	}
	if (m == selmon)
		return;
	unfocus(selmon->sel, 0);
	selmon = m;
	warptomon(selmon);
	focus(NULL);
}

void
warptoclient(Client *c)
{
	/* move the pointer onto the focused client so focus-follows-mouse agrees
	 * with an explicit CLI focus change instead of overriding it */
	if (!warpcursor || !c)
		return;
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w / 2, c->h / 2);
}

void
grabbuttons(Client *c, int focused)
{
	/* While unfocused, grab clicks so the first click focuses the window
	 * (buttonpress() replays it to the app). Once focused, release the grab so
	 * clicks go straight to the client — exactly like dwm. */
	XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
	if (!focused) {
		XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
		            ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
	} else {
		/* on the focused window, plain clicks pass through to the app;
		 * only movemod + Btn1/Btn3 are grabbed for move/resize */
		XGrabButton(dpy, Button1, movemod, c->win, False,
		            ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
		XGrabButton(dpy, Button3, movemod, c->win, False,
		            ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
	}
}

void
warptomon(Monitor *m)
{
	/* move the pointer to the centre of m so focus-follows-mouse agrees with
	 * an explicit CLI monitor switch instead of immediately overriding it */
	if (!warpcursor || !m)
		return;
	XWarpPointer(dpy, None, root, 0, 0, 0, 0, m->mx + m->mw / 2, m->my + m->mh / 2);
}

Monitor *
dirtomon(int dir)
{
	Monitor *m = NULL;

	if (dir > 0) {
		if (!(m = selmon->next))
			m = mons;
	} else if (selmon == mons) {
		for (m = mons; m->next; m = m->next)
			;
	} else {
		for (m = mons; m->next != selmon; m = m->next)
			;
	}
	return m;
}

/* ---- tags / monitors actions ---------------------------------------- */

/* parse a tag argument that is a name or 1-based number; cur is fallback */
int
settag(const char *arg, int cur)
{
	unsigned int i;
	char buf[64];
	int n;

	for (i = 0; i < LENGTH(tags); i++)
		if (!strcmp(arg, tags[i]))
			return i;
	/* numeric: accept the printable label too */
	n = atoi(arg);
	if (n >= 1 && n <= MAX_TAGS)
		return n - 1;
	for (i = 0; i < MAX_TAGS; i++) {
		taglabel(i, buf, sizeof buf);
		if (!strcmp(arg, buf))
			return i;
	}
	return cur;
}

void
viewtag(const char *arg)
{
	int t, cur = selmon->seltag;
	int i;
	Monitor *o, *old = selmon;

	if (!strcmp(arg, "next") || !strcmp(arg, "prev")) {
		int dir = arg[0] == 'n' ? 1 : -1;
		t = cur < 0 ? (dir > 0 ? -1 : tagcount()) : cur;
		for (i = 0; i < tagcount(); i++) {
			t = (t + dir + tagcount()) % tagcount();
			/* land on the next tag that is free or already ours; skip tags
			 * currently shown on another monitor, then claim a free one */
			if (tagstate[t].mon < 0 || tagstate[t].mon == selmon->num) {
				if (tagstate[t].mon < 0)
					tagstate[t].mon = selmon->num;
				selmon->seltag = t;
				goto done;
			}
		}
		return; /* every other tag belongs to another monitor */
	}
	t = settag(arg, cur);
	if (t < 0)
		return;
	o = montag(t);
	if (!o) {
		/* unowned: claim it for the focused monitor */
		tagstate[t].mon = selmon->num;
		selmon->seltag = t;
	} else {
		o->seltag = t;
		selmon = o;
	}
done:
	if (selmon != old)
		warptomon(selmon);
	arrange(NULL);
	focus(NULL);
	drawbars();
}

void
sendmon(Client *c, int tag)
{
	if (c->tag == tag)
		return;
	unfocus(c, 1);
	c->tag = tag;
	focus(NULL);
	arrange(NULL);
}

void
movewintotag(const char *arg)
{
	Client *c = selmon->sel;
	int t, cur;

	if (!c)
		return;
	cur = c->tag;
	if (!strcmp(arg, "next"))
		t = (cur + 1) % tagcount();
	else if (!strcmp(arg, "prev"))
		t = (cur - 1 + tagcount()) % tagcount();
	else
		t = settag(arg, cur);
	if (t == cur)
		return;
	/* ensure target tag has an owner monitor (else it would vanish) */
	if (tagstate[t].mon < 0)
		tagstate[t].mon = selmon->num;
	sendmon(c, t);
	drawbars();
}

void
movetagtomon(const char *arg)
{
	int t = selmon->seltag, target, n;
	Monitor *m, *src = selmon;

	if (t < 0 || !mons->next)
		return;
	n = 0;
	for (m = mons; m; m = m->next)
		n++;
	if (!strcmp(arg, "next"))
		target = (selmon->num + 1) % n;
	else if (!strcmp(arg, "prev"))
		target = (selmon->num - 1 + n) % n;
	else {
		target = atoi(arg);
		if (target < 0 || target >= n)
			return;
	}
	if (target == src->num)
		return;

	tagstate[t].mon = target;
	/* source monitor falls back to another owned tag, or nothing */
	src->seltag = -1;
	{
		int i;
		for (i = 0; i < MAX_TAGS; i++)
			if (tagstate[i].mon == src->num) {
				src->seltag = i;
				break;
			}
	}
	/* show the moved tag on the target and follow it */
	for (m = mons; m && m->num != target; m = m->next)
		;
	if (m) {
		m->seltag = t;
		selmon = m;
		warptomon(selmon);
	}
	arrange(NULL);
	focus(NULL);
	drawbars();
}

/* ---- layout / geometry actions -------------------------------------- */

void
setlayout(const char *arg)
{
	int t = selmon->seltag, l;

	if (t < 0)
		return;
	if (!strcmp(arg, "cols"))
		l = 0;
	else if (!strcmp(arg, "stack"))
		l = 1;
	else if (!strcmp(arg, "float"))
		l = 2;
	else if (!strcmp(arg, "next"))
		l = (tagstate[t].layout + 1) % 3;
	else if (!strcmp(arg, "prev"))
		l = (tagstate[t].layout + 2) % 3;
	else
		return;
	tagstate[t].layout = l;
	arrange(selmon);
	drawbars();
}

void
incnmaster(const char *arg)
{
	int t = selmon->seltag, v;

	if (t < 0)
		return;
	if (arg[0] == '+' || arg[0] == '-')
		v = tagstate[t].nmaster + atoi(arg);
	else
		v = atoi(arg);
	tagstate[t].nmaster = MAX(v, 0);
	arrange(selmon);
}

void
setmfact(const char *arg)
{
	int t = selmon->seltag;
	float f;

	if (t < 0)
		return;
	f = atof(arg);
	if (arg[0] == '+' || arg[0] == '-')
		f += tagstate[t].mfact;
	if (f < 0.05f) f = 0.05f;
	if (f > 0.95f) f = 0.95f;
	tagstate[t].mfact = f;
	arrange(selmon);
}

void
setgap(const char *arg)
{
	int v;

	if (arg[0] == '+' || arg[0] == '-')
		v = gappx_run + atoi(arg);
	else
		v = atoi(arg);
	gappx_run = MAX(v, 0);
	arrange(NULL);
}

static int barvisible = 1;
static int systrayvisible = 1;

void
togglebar(void)
{
	Monitor *m;

	barvisible = !barvisible;
	for (m = mons; m; m = m->next) {
		if (barvisible) {
			XMapRaised(dpy, m->barwin);
			m->wh = m->mh - bh - barvmargin;
			if (topbar) {
				m->by = m->my + barvmargin;
				m->wy = m->my + bh + barvmargin;
			} else {
				m->by = m->my + m->mh - bh - barvmargin;
				m->wy = m->my;
			}
			XMoveResizeWindow(dpy, m->barwin, m->mx, m->by, m->mw, bh);
		} else {
			XUnmapWindow(dpy, m->barwin);
			m->by = -bh;
			m->wy = m->my;
			m->wh = m->mh;
		}
	}
	if (showsystray)
		updatesystray();
	arrange(NULL);
}

void
toggletraybar(void)
{
	if (!showsystray)
		return;
	systrayvisible = !systrayvisible;
	/* drawbar reclaims the status pill's space (getsystraywidth -> 0 when
	 * hidden) and updatesystray (un)maps the tray window + its icons */
	drawbars();
}

/* ---- client state actions ------------------------------------------- */

void
zoom(void)
{
	Client *c = selmon->sel;

	if (!c || c->isfloating || c->isfullscreen)
		return;
	detach(c);
	attach(c); /* move to head -> becomes master */
	arrange(selmon);
	focus(c);
}

void
movemouse(void)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel) || c->isfullscreen)
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
	                 None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;
	if (!c->isfloating) {
		c->isfloating = 1;
		arrange(selmon);
	}
	do {
		XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;
			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if ((m = clientmon(c))) {
				if (abs(m->wx - nx) < (int)snap)
					nx = m->wx;
				else if (abs((m->wx + m->ww) - (nx + WIDTH(c))) < (int)snap)
					nx = m->wx + m->ww - WIDTH(c);
				if (abs(m->wy - ny) < (int)snap)
					ny = m->wy;
				else if (abs((m->wy + m->wh) - (ny + HEIGHT(c))) < (int)snap)
					ny = m->wy + m->wh - HEIGHT(c);
			}
			resize(c, nx, ny, c->w, c->h, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	/* dragged onto another monitor: hand the window to that monitor's tag */
	if ((m = recttomon(c->x + c->w / 2, c->y + c->h / 2, 1, 1)) != selmon && m->seltag >= 0) {
		sendmon(c, m->seltag);
		selmon = m;
		focus(c);
	}
}

void
resizemouse(void)
{
	int ocx, ocy, nw, nh;
	Client *c;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel) || c->isfullscreen)
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
	                 None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!c->isfloating) {
		c->isfloating = 1;
		arrange(selmon);
	}
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	do {
		XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;
			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
			resize(c, c->x, c->y, nw, nh, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev))
		;
}

void
focuslast(void)
{
	Client *c;

	for (c = fstack; c; c = c->snext)
		if (ISVISIBLE(c) && clientmon(c) == selmon && c != selmon->sel) {
			focus(c);
			restack(selmon);
			warptoclient(c);
			return;
		}
}

void
movewinmon(int dir)
{
	Client *c = selmon->sel;
	Monitor *m;
	int t, i;

	if (!c || !mons->next)
		return;
	m = dirtomon(dir);
	if (!m || m == selmon)
		return;
	t = m->seltag;
	if (t < 0) {
		for (i = 0, t = -1; i < MAX_TAGS; i++)
			if (tagstate[i].mon < 0) { t = i; break; }
		if (t < 0)
			t = 0;
		tagstate[t].mon = m->num;
		m->seltag = t;
	}
	sendmon(c, t);
	drawbars();
}

void
winmove(int dx, int dy)
{
	Client *c = selmon->sel;

	if (!c || c->isfullscreen)
		return;
	if (!c->isfloating) {
		c->isfloating = 1;
		arrange(selmon);
	}
	resize(c, c->x + dx, c->y + dy, c->w, c->h, 1);
	restack(selmon);
}

void
winresize(int dw, int dh)
{
	Client *c = selmon->sel;

	if (!c || c->isfullscreen)
		return;
	if (!c->isfloating) {
		c->isfloating = 1;
		arrange(selmon);
	}
	resize(c, c->x, c->y, MAX(c->w + dw, 1), MAX(c->h + dh, 1), 1);
	restack(selmon);
}

void
wincenter(void)
{
	Client *c = selmon->sel;
	Monitor *m;

	if (!c || c->isfullscreen)
		return;
	if (!c->isfloating) {
		c->isfloating = 1;
		arrange(selmon);
	}
	m = clientmon(c);
	if (!m)
		m = selmon;
	resize(c, m->wx + (m->ww - WIDTH(c)) / 2, m->wy + (m->wh - HEIGHT(c)) / 2, c->w, c->h, 1);
	restack(selmon);
}

void
togglefloating(void)
{
	Client *c = selmon->sel;

	if (!c || c->isfullscreen)
		return;
	c->isfloating = !c->isfloating || c->isfixed;
	if (c->isfloating)
		resize(c, c->x, c->y, c->w, c->h, 0);
	arrange(selmon);
}

void
togglefullscreen(void)
{
	if (selmon->sel)
		setfullscreen(selmon->sel, !selmon->sel->isfullscreen);
}

void
togglesticky(void)
{
	Client *c = selmon->sel;
	if (!c)
		return;
	c->issticky = !c->issticky;
	if (c->issticky)
		c->isfloating = 1;
	arrange(selmon);
	drawbars();
}

void
killfocused(void)
{
	if (!selmon->sel)
		return;
	if (!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0, 0, 0)) {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
setfullscreen(Client *c, int fullscreen)
{
	Monitor *m = clientmon(c);

	if (fullscreen && !c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
		                PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;
		c->oldstate = c->isfloating;
		c->oldbw = c->bw;
		c->bw = 0;
		c->isfloating = 1;
		if (m) {
			clearround(c->win);
			resizeclient(c, m->mx, m->my, m->mw, m->mh);
		}
		XRaiseWindow(dpy, c->win);
	} else if (!fullscreen && c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
		                PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		c->isfloating = c->oldstate;
		c->bw = c->oldbw;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		arrange(clientmon(c));
	}
}

void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	c->isurgent = urg;
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

/* ---- resize / restack ----------------------------------------------- */

void
resize(Client *c, int x, int y, int w, int h, int interact)
{
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	if (roundcorners && !c->isfullscreen)
		roundwin(c->win, w, h, cornerradius, c->bw);
	else
		clearround(c->win);
	XSync(dpy, False);
}

void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if (!m->sel)
		return;
	if (m->sel->isfloating || m->sel->isfullscreen)
		XRaiseWindow(dpy, m->sel->win);
	/* raise the bar above tiled clients */
	wc.stack_mode = Below;
	wc.sibling = m->barwin;
	for (c = fstack; c; c = c->snext)
		if (!c->isfloating && ISVISIBLE(c) && clientmon(c) == m) {
			XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
			wc.sibling = c->win;
		}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev))
		;
}

void
showhide(Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((tagstate[c->tag].layout == 2 || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

/* ---- rounded corners (X Shape) -------------------------------------- */

/* fill a rounded rectangle of 1s into a 1-bit shape mask at (x,y) */
void
maskroundrect(Pixmap mask, GC g, int x, int y, int w, int h, int rad)
{
	int d;

	if (w <= 0 || h <= 0)
		return;
	if (rad < 0) rad = 0;
	if (rad * 2 > w) rad = w / 2;
	if (rad * 2 > h) rad = h / 2;
	if (rad == 0) {
		XFillRectangle(dpy, mask, g, x, y, w, h);
		return;
	}
	d = rad * 2;
	XFillRectangle(dpy, mask, g, x + rad, y, w - d, h);
	XFillRectangle(dpy, mask, g, x, y + rad, w, h - d);
	XFillArc(dpy, mask, g, x,         y,         d, d, 0, 360 * 64);
	XFillArc(dpy, mask, g, x + w - d, y,         d, d, 0, 360 * 64);
	XFillArc(dpy, mask, g, x,         y + h - d, d, d, 0, 360 * 64);
	XFillArc(dpy, mask, g, x + w - d, y + h - d, d, d, 0, 360 * 64);
}

void
roundwin(Window win, int w, int h, int rad, int bw)
{
	Pixmap mask;
	GC g;
	int W = w + 2 * bw, H = h + 2 * bw;

	if (!shapeext || rad <= 0 || W <= 0 || H <= 0)
		return;
	/* The ShapeBounding region must include the border, which sits OUTSIDE the
	 * w*h content (at window-relative offset -bw). A content-only mask shapes
	 * the border away entirely, so it never renders. Build a mask covering
	 * content+border and combine it at offset -bw so it starts at the border's
	 * outer edge; round the outer corners by rad+bw to keep the border even. */
	mask = XCreatePixmap(dpy, win, W, H, 1);
	g = XCreateGC(dpy, mask, 0, NULL);
	XSetForeground(dpy, g, 0);
	XFillRectangle(dpy, mask, g, 0, 0, W, H);
	XSetForeground(dpy, g, 1);
	maskroundrect(mask, g, 0, 0, W, H, rad + bw);
	XShapeCombineMask(dpy, win, ShapeBounding, -bw, -bw, mask, ShapeSet);
	XFreeGC(dpy, g);
	XFreePixmap(dpy, mask);
}

void
clearround(Window w)
{
	if (shapeext)
		XShapeCombineMask(dpy, w, ShapeBounding, 0, 0, None, ShapeSet);
}

/* Shape the bar window down to just its pill rectangles so the gaps between
 * pills are not part of the window at all — the wallpaper shows through there
 * untouched (no fill, no compositor blur). Rebuilt on every drawbar() since the
 * pills move. Without the Shape extension the bar stays a full rectangle. */
void
shapebar(Monitor *m, int rects[][2], int n)
{
	Pixmap mask;
	GC g;
	int i;

	if (!shapeext || (int)m->mw <= 0 || bh <= 0)
		return;
	mask = XCreatePixmap(dpy, m->barwin, m->mw, bh, 1);
	g = XCreateGC(dpy, mask, 0, NULL);
	XSetForeground(dpy, g, 0);
	XFillRectangle(dpy, mask, g, 0, 0, m->mw, bh);
	XSetForeground(dpy, g, 1);
	for (i = 0; i < n; i++)
		maskroundrect(mask, g, rects[i][0], 0, rects[i][1], bh, segradius);
	XShapeCombineMask(dpy, m->barwin, ShapeBounding, 0, 0, mask, ShapeSet);
	XFreeGC(dpy, g);
	XFreePixmap(dpy, mask);
}

/* ---- the bar --------------------------------------------------------- */

void
drawbar(Monitor *m)
{
	int x, w, lw, cw, rw, sysw = 0, rightedge;
	int i;
	char buf[64];
	Client *c;
	int pills[3][2]; /* {x, width} of each drawn pill, for window shaping */
	int np = 0;

	if (m->by < 0)
		return;

	/* clear the whole bar to fully transparent; only the pills get painted,
	 * and the window is shaped to them below so the gaps between pills stay
	 * out of the bar window entirely (wallpaper/compositor untouched there) */
	XSetForeground(dpy, drw->gc, 0);
	XFillRectangle(dpy, drw->drawable, drw->gc, 0, 0, m->mw, bh);

	if (showsystray && m->num == systraymon)
		sysw = getsystraywidth();

	/* ---- LEFT: tags + layout symbol ---- */
	lw = 0;
	for (i = 0; i < tagcount(); i++) {
		if (tagstate[i].mon != m->num)
			continue;
		if (!occupied(i) && i != m->seltag)
			continue;
		taglabel(i, buf, sizeof buf);
		lw += TEXTW(buf);
	}
	if (m->seltag >= 0 && m->ltsymbol[0])
		lw += TEXTW(m->ltsymbol);

	if (lw > 0) {
		pills[np][0] = barmargin; pills[np][1] = lw; np++;
		drw_roundrect(drw, barmargin, 0, lw, bh, segradius, &fillcol[SchemeNorm]);
		x = barmargin;
		for (i = 0; i < tagcount(); i++) {
			if (tagstate[i].mon != m->num)
				continue;
			if (!occupied(i) && i != m->seltag)
				continue;
			taglabel(i, buf, sizeof buf);
			w = TEXTW(buf);
			if (i == m->seltag)
				drw_roundrect(drw, x, 0, w, bh, segradius, &fillcol[SchemeSel]);
			else {
				int urg = 0;
				for (c = clients; c; c = c->next)
					if (c->tag == i && c->isurgent) { urg = 1; break; }
				if (urg)
					drw_roundrect(drw, x, 0, w, bh, segradius, &fillcol[SchemeUrg]);
			}
			drw_setscheme(drw, scheme[i == m->seltag ? SchemeSel : SchemeNorm]);
			drw_text(drw, x, 0, w, bh, lrpad / 2, buf, 0);
			x += w;
		}
		if (m->seltag >= 0 && m->ltsymbol[0]) {
			w = TEXTW(m->ltsymbol);
			drw_setscheme(drw, scheme[SchemeNorm]);
			drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);
		}
	}

	/* ---- RIGHT: status text ---- */
	rw = stext[0] ? TEXTW(stext) : 0;
	rightedge = m->mw - barmargin - (sysw ? sysw + barseggap : 0);
	if (rw > 0) {
		if (rw > rightedge - barmargin)
			rw = rightedge - barmargin;
		pills[np][0] = rightedge - rw; pills[np][1] = rw; np++;
		drw_roundrect(drw, rightedge - rw, 0, rw, bh, segradius, &fillcol[SchemeStatus]);
		drw_setscheme(drw, scheme[SchemeStatus]);
		drw_text(drw, rightedge - rw, 0, rw, bh, lrpad / 2, stext, 0);
	}

	/* ---- CENTER: focused window title ---- */
	if (m->sel && m->sel->name[0]) {
		int lo = barmargin + lw + (lw ? barseggap : 0);
		int hi = rightedge - (rw ? rw + barseggap : 0);
		cw = TEXTW(m->sel->name);
		if (cw > hi - lo)
			cw = hi - lo;
		if (cw > 0) {
			int cx = titlealign == 0 ? lo
			       : titlealign == 2 ? hi - cw
			       : lo + (hi - lo - cw) / 2;
			pills[np][0] = cx; pills[np][1] = cw; np++;
			drw_roundrect(drw, cx, 0, cw, bh, segradius, &fillcol[SchemeTitle]);
			drw_setscheme(drw, scheme[SchemeTitle]);
			drw_text(drw, cx, 0, cw, bh, lrpad / 2, m->sel->name, 0);
		}
	}

	drw_map(drw, m->barwin, 0, 0, m->mw, bh);
	shapebar(m, pills, np);
}

void
drawbars(void)
{
	Monitor *m;
	for (m = mons; m; m = m->next)
		drawbar(m);
	if (showsystray)
		updatesystray();
}

void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	if (showbar) {
		m->wh -= bh + barvmargin;
		if (topbar) {
			m->by = m->my + barvmargin;
			m->wy = m->my + bh + barvmargin;
		} else {
			m->by = m->my + m->mh - bh - barvmargin;
			m->wy = m->my;
		}
	} else {
		m->by = -bh;
	}
}

/* ---- events ---------------------------------------------------------- */

void
buttonpress(XEvent *e)
{
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;

	m = wintomon(ev->window);
	if (m && m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	if ((c = wintoclient(ev->window))) {
		focus(c);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		if (ev->state & movemod) {
			if (ev->button == Button1)
				movemouse();
			else if (ev->button == Button3)
				resizemouse();
		}
		return;
	}
	/* forward button press on the tray window to the icon under the cursor */
	if (systray && ev->window == systray->win) {
		Client *i;
		int off = lrpad / 2;
		for (i = systray->icons; i; i = i->next) {
			if (ev->x >= off && ev->x < off + (int)i->w
			    && ev->y >= systrayvertpad && ev->y < systrayvertpad + (int)i->h) {
				XButtonEvent fwd = *ev;
				fwd.window = i->win;
				fwd.x -= off;
				fwd.y -= systrayvertpad;
				fwd.subwindow = None;
				XSendEvent(dpy, i->win, True, ButtonPressMask, (XEvent *)&fwd);
				XSync(dpy, False);
				return;
			}
			off += i->w + systrayspacing;
		}
		return;
	}
	if (m && ev->window == m->barwin) {
		int x = barmargin, i;
		char buf[64];

		selmon = m;
		if (ev->button == Button4 || ev->button == Button5) {
			viewtag(ev->button == Button4 ? "prev" : "next");
			return;
		}
		for (i = 0; i < tagcount(); i++) {
			if (tagstate[i].mon != m->num)
				continue;
			if (!occupied(i) && i != m->seltag)
				continue;
			taglabel(i, buf, sizeof buf);
			if (ev->x >= x && ev->x < x + TEXTW(buf)) {
				m->seltag = i;
				arrange(NULL);
				focus(NULL);
				drawbars();
				return;
			}
			x += TEXTW(buf);
		}
		if (m->seltag >= 0 && m->ltsymbol[0] && ev->x >= x && ev->x < x + TEXTW(m->ltsymbol))
			setlayout(ev->button == Button3 ? "prev" : "next");
	}
}

void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	if (!focusfollowsmouse)
		return;
	c = wintoclient(ev->window);
	m = c ? clientmon(c) : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m ? m : selmon;
	}
	if (c && c != selmon->sel)
		focus(c);
}

void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	if (ev->window != root)
		return;
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	mon = m;
}

void
expose(XEvent *e)
{
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0 && (m = wintomon(ev->window)))
		drawbar(m);
}

void
configurenotify(XEvent *e)
{
	Monitor *m;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;

	if (ev->window == root) {
		sw = ev->width;
		sh = ev->height;
		if (updategeom(0)) {
			drw_resize(drw, sw, bh);
			for (m = mons; m; m = m->next) {
				for (c = clients; c; c = c->next)
					if (c->isfullscreen && clientmon(c) == m)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
				XMoveResizeWindow(dpy, m->barwin, m->mx, m->by, m->mw, bh);
			}
			focus(NULL);
			arrange(NULL);
		}
	}
}

void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if (c->isfloating) {
			m = clientmon(c);
			if (ev->value_mask & CWX) { c->oldx = c->x; c->x = (m ? m->mx : 0) + ev->x; }
			if (ev->value_mask & CWY) { c->oldy = c->y; c->y = (m ? m->my : 0) + ev->y; }
			if (ev->value_mask & CWWidth) { c->oldw = c->w; c->w = ev->width; }
			if (ev->value_mask & CWHeight) { c->oldh = c->h; c->h = ev->height; }
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

void
configure(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);
	else if ((c = wintosystrayicon(ev->window))) {
		removesystrayicon(c);
		updatesystray();
	}
}

void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
	} else if ((c = wintosystrayicon(ev->window))) {
		/* KDE/GTK panel icons sometimes unmap themselves; re-embed */
		removesystrayicon(c);
		updatesystray();
	}
}

void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;
	Client *i;

	if ((i = wintosystrayicon(ev->window))) {
		sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
		updatesystray();
	}
	if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;
	XRefreshKeyboardMapping(ev);
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if ((c = wintosystrayicon(ev->window))) {
		if (ev->atom == XA_WM_NORMAL_HINTS) {
			updatesizehints(c);
			updatesystrayicongeom(c, c->w, c->h);
		} else
			updatesystrayiconstate(c, ev);
		updatesystray();
	}
	if ((ev->window == root) && (ev->atom == XA_WM_NAME)) {
		updatestatus();
	} else if (ev->state == PropertyDelete) {
		return;
	} else if ((c = wintoclient(ev->window))) {
		switch (ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
			    (c->isfloating = (wintoclient(trans)) != NULL))
				arrange(clientmon(c));
			break;
		case XA_WM_NORMAL_HINTS:
			c->hintsvalid = 0;
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == clientmon(c)->sel)
				drawbar(clientmon(c));
		}
		if (ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

void
resizerequest(XEvent *e)
{
	XResizeRequestEvent *ev = &e->xresizerequest;
	Client *i;

	if ((i = wintosystrayicon(ev->window))) {
		updatesystrayicongeom(i, ev->width, ev->height);
		updatesystray();
	}
}

void
clientmessage(XEvent *e)
{
	XWindowAttributes wa;
	XClientMessageEvent *cme = &e->xclient;
	Client *c;

	/* mwmc IPC */
	if (cme->message_type == mwmatom[MwmMsg]) {
		handleipc((Window)cme->data.l[0]);
		return;
	}

	/* system tray dock requests */
	if (showsystray && systray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
		if (cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
			if (!(c = (Client *)calloc(1, sizeof(Client))))
				die("calloc:");
			if (!(c->win = cme->data.l[2])) {
				free(c);
				return;
			}
			c->tag = -1;
			c->next = systray->icons;
			systray->icons = c;
			if (!XGetWindowAttributes(dpy, c->win, &wa)) {
				wa.width = bh;
				wa.height = bh;
				wa.border_width = 0;
			}
			c->x = c->oldx = c->y = c->oldy = 0;
			c->w = c->oldw = wa.width;
			c->h = c->oldh = wa.height;
			c->oldbw = wa.border_width;
			c->bw = 0;
			c->isfloating = 1;
			updatesizehints(c);
			updatesystrayicongeom(c, wa.width, wa.height);
			XAddToSaveSet(dpy, c->win);
			{
			XWindowAttributes wa_ev;
			if (XGetWindowAttributes(dpy, c->win, &wa_ev))
				XSelectInput(dpy, c->win, wa_ev.your_event_mask
				             | StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
			else
				XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
		}
			XReparentWindow(dpy, c->win, systray->win, 0, 0);
			/* inherit the tray window's background so the icon's surround is
			 * exactly the tray-bar colour (XEmbed standard) */
			XSetWindowBackgroundPixmap(dpy, c->win, ParentRelative);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0, systray->win, XEMBED_EMBEDDED_VERSION);
			XSync(dpy, False);
			setclientstate(c, NormalState);
			updatesystray();
		}
		return;
	}

	if (!(c = wintoclient(cme->window)))
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen] || cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD */
			    || (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
		else if (cme->data.l[1] == netatom[NetWMStateDemandsAttention] || cme->data.l[2] == netatom[NetWMStateDemandsAttention]) {
			if (c != selmon->sel && !c->isurgent)
				seturgent(c, 1);
		}
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (c != selmon->sel && !c->isurgent)
			seturgent(c, 1);
	}
}

/* ---- IPC dispatch ---------------------------------------------------- */

void
spawnreply(Window w, const char *s)
{
	XChangeProperty(dpy, w, mwmatom[MwmReply], xa_utf8, 8, PropModeReplace,
	                (const unsigned char *)s, strlen(s));
	XSync(dpy, False);
}

void
handleipc(Window requestor)
{
	Atom type;
	int fmt, argc = 0;
	unsigned long nitems, after;
	unsigned char *data = NULL;
	char *argv[32];
	char *p, *end;
	char *reply;
	const size_t rsz = 1 << 16;

	if (XGetWindowProperty(dpy, requestor, mwmatom[MwmCmd], 0, (1 << 16), False,
	                       AnyPropertyType, &type, &fmt, &nitems, &after, &data) != Success || !data) {
		spawnreply(requestor, "error: no command");
		return;
	}
	/* split the NUL-separated argv */
	p = (char *)data;
	end = p + nitems;
	while (p < end && argc < (int)LENGTH(argv)) {
		argv[argc++] = p;
		p += strlen(p) + 1;
	}
	reply = ecalloc(1, rsz);
	reply[0] = '\0';
	if (argc > 0)
		dispatchcmd(argc, argv, reply, rsz);
	else
		snprintf(reply, rsz, "error: empty command");
	spawnreply(requestor, reply);
	free(reply);
	XFree(data);
	XDeleteProperty(dpy, requestor, mwmatom[MwmCmd]);
}

#define ARG(i)   ((i) < argc ? argv[i] : "")
#define EQ(a,b)  (!strcmp((a),(b)))

void
dispatchcmd(int argc, char **argv, char *reply, size_t rsz)
{
	const char *verb = argv[0];
	const char *noun = ARG(1);
	size_t n = 0;

	if (EQ(verb, "focus")) {
		if (EQ(noun, "next")) focusstack(+1);
		else if (EQ(noun, "prev")) focusstack(-1);
		else if (EQ(noun, "last")) focuslast();
		else if (EQ(noun, "win")) {
			if (EQ(ARG(2), "next")) focusstack(+1);
			else if (EQ(ARG(2), "prev")) focusstack(-1);
			else if (EQ(ARG(2), "last")) focuslast();
			else if (argc > 2) focusclientbyclass(argv[2]);
			else { snprintf(reply, rsz, "error: focus win next|prev|last|<class>"); return; }
		} else if (EQ(noun, "tag")) {
			if (argc > 2) viewtag(argv[2]);
			else { snprintf(reply, rsz, "error: focus tag next|prev|<name>"); return; }
		} else if (EQ(noun, "mon")) {
			if (argc > 2) focusmon(argv[2]);
			else { snprintf(reply, rsz, "error: focus mon next|prev|<n>"); return; }
		} else { snprintf(reply, rsz, "error: focus next|prev|win|tag|mon ..."); return; }
	} else if (EQ(verb, "move")) {
		if (EQ(noun, "win")) {
			if (EQ(ARG(2), "master")) zoom();
			else if (EQ(ARG(2), "mon")) {
				if (EQ(ARG(3), "next")) movewinmon(+1);
				else if (EQ(ARG(3), "prev")) movewinmon(-1);
				else { snprintf(reply, rsz, "error: move win mon next|prev"); return; }
			}
			else if (argc > 2) movewintotag(argv[2]);
			else { snprintf(reply, rsz, "error: move win next|prev|master|mon|<tag>"); return; }
		} else if (EQ(noun, "tag")) {
			if (argc > 2) movetagtomon(argv[2]);
			else { snprintf(reply, rsz, "error: move tag next|prev|<mon>"); return; }
		} else { snprintf(reply, rsz, "error: move win|tag ..."); return; }
	} else if (EQ(verb, "swap")) {
		/* swap focused window forward/back in the stack order */
		Client *c = selmon->sel, *nb = NULL;
		if (!c || c->isfloating) { snprintf(reply, rsz, "error: no tiled window"); return; }
		if (EQ(noun, "next")) {
			nb = nexttiled(c->next, selmon, c->tag);
			if (nb) { detach(c); attachafter(c, nb); }
		} else if (EQ(noun, "prev")) {
			Client *i;
			for (i = clients; i && i != c; i = i->next)
				if (i->tag == c->tag && !i->isfloating && ISVISIBLE(i)) nb = i;
			if (nb) {
				Client *before = NULL, *j;
				for (j = clients; j && j != nb; j = j->next) before = j;
				detach(c); attachafter(c, before);
			}
		} else { snprintf(reply, rsz, "error: swap next|prev"); return; }
		arrange(selmon); focus(c);
	} else if (EQ(verb, "layout")) {
		if (argc > 1) setlayout(noun);
		else { snprintf(reply, rsz, "error: layout cols|stack|float|next|prev"); return; }
	} else if (EQ(verb, "master")) {
		if (EQ(noun, "ratio")) {
			if (argc > 2) setmfact(argv[2]);
			else { snprintf(reply, rsz, "error: master ratio +0.05|-0.05|<f>"); return; }
		} else if (argc > 1) incnmaster(noun);
		else { snprintf(reply, rsz, "error: master +1|-1|<n>|ratio ..."); return; }
	} else if (EQ(verb, "gap")) {
		if (argc > 1) setgap(noun);
		else { snprintf(reply, rsz, "error: gap +5|-5|<px>"); return; }
	} else if (EQ(verb, "bar")) {
		if (EQ(noun, "toggle")) togglebar();
		else if (EQ(noun, "show")) { if (!barvisible) togglebar(); }
		else if (EQ(noun, "hide")) { if (barvisible) togglebar(); }
		else { snprintf(reply, rsz, "error: bar toggle|show|hide"); return; }
	} else if (EQ(verb, "traybar")) {
		if (EQ(noun, "toggle")) toggletraybar();
		else if (EQ(noun, "show")) { if (!systrayvisible) toggletraybar(); }
		else if (EQ(noun, "hide")) { if (systrayvisible) toggletraybar(); }
		else { snprintf(reply, rsz, "error: traybar toggle|show|hide"); return; }
	} else if (EQ(verb, "win")) {
		if (EQ(noun, "close")) killfocused();
		else if (EQ(noun, "move")) {
			if (argc > 3) winmove(atoi(argv[2]), atoi(argv[3]));
			else { snprintf(reply, rsz, "error: win move <dx> <dy>"); return; }
		} else if (EQ(noun, "resize")) {
			if (argc > 3) winresize(atoi(argv[2]), atoi(argv[3]));
			else { snprintf(reply, rsz, "error: win resize <dw> <dh>"); return; }
		} else if (EQ(noun, "center")) {
			wincenter();
		} else if (EQ(noun, "tag")) {
			if (argc > 2) movewintotag(argv[2]);
			else { snprintf(reply, rsz, "error: win tag <name>"); return; }
		} else if (EQ(noun, "toggle")) {
			if (EQ(ARG(2), "float")) togglefloating();
			else if (EQ(ARG(2), "full")) togglefullscreen();
			else if (EQ(ARG(2), "sticky")) togglesticky();
			else { snprintf(reply, rsz, "error: win toggle float|full|sticky"); return; }
		} else { snprintf(reply, rsz, "error: win close|tag|toggle ..."); return; }
	} else if (EQ(verb, "kill")) {
		killfocused();
	} else if (EQ(verb, "reload")) {
		const char *err = reloadstyle();
		if (err) {
			snprintf(reply, rsz, "error: %s", err);
			return;
		}
	} else if (EQ(verb, "quit")) {
		running = 0;
	} else if (EQ(verb, "version")) {
		snprintf(reply, rsz, "mwm-%s", VERSION);
		return;
	} else if (EQ(verb, "query")) {
		Monitor *m;
		Client *c;
		char lbl[64];
		int i;
		if (EQ(noun, "tags")) {
			for (i = 0; i < tagcount() && n < rsz - 128; i++) {
				if (tagstate[i].mon < 0 && !occupied(i))
					continue;
				taglabel(i, lbl, sizeof lbl);
				n += snprintf(reply + n, rsz - n, "tag %d %s mon=%d sel=%d occ=%d layout=%s\n",
				              i, lbl, tagstate[i].mon,
				              (montag(i) && montag(i)->seltag == i),
				              occupied(i), layoutsym[tagstate[i].layout]);
			}
		} else if (EQ(noun, "windows")) {
			for (c = clients; c && n < rsz - 320; c = c->next)
				n += snprintf(reply + n, rsz - n, "win 0x%lx tag=%d class=%s float=%d full=%d sticky=%d focused=%d \"%s\"\n",
				              c->win, c->tag, c->class, c->isfloating, c->isfullscreen,
				              c->issticky, c == selmon->sel, c->name);
		} else if (EQ(noun, "monitors")) {
			for (m = mons; m && n < rsz - 128; m = m->next)
				n += snprintf(reply + n, rsz - n, "mon %d %dx%d+%d+%d seltag=%d sel=%d\n",
				              m->num, m->mw, m->mh, m->mx, m->my, m->seltag, m == selmon);
		} else if (EQ(noun, "layout")) {
			int t = selmon->seltag;
			snprintf(reply, rsz, "%s\n", t >= 0 ? layoutsym[tagstate[t].layout] : "-");
		} else if (EQ(noun, "state")) {
			int t = selmon->seltag;
			taglabel(t >= 0 ? t : 0, lbl, sizeof lbl);
			snprintf(reply, rsz, "mon=%d tag=%s layout=%s nmaster=%d mfact=%.2f gap=%d sel=%s\n",
			         selmon->num, t >= 0 ? lbl : "-", t >= 0 ? layoutsym[tagstate[t].layout] : "-",
			         t >= 0 ? tagstate[t].nmaster : 0, t >= 0 ? tagstate[t].mfact : 0.0,
			         gappx_run, selmon->sel ? selmon->sel->name : "");
		} else {
			snprintf(reply, rsz, "error: query tags|windows|monitors|layout|state");
		}
		return;
	} else if (EQ(verb, "help") || EQ(verb, "-h") || EQ(verb, "--help")) {
		snprintf(reply, rsz,
		    "mwm commands (window <= tag <= monitor):\n"
		    "  focus next|prev|last | focus win next|prev|last|<class>\n"
		    "  focus tag next|prev|<name> | focus mon next|prev|<n>\n"
		    "  move win next|prev|master|<tag> | move win mon next|prev\n"
		    "  move tag next|prev|<mon> | swap next|prev\n"
		    "  layout cols|stack|float|next|prev\n"
		    "  master +1|-1|<n> | master ratio +0.05|-0.05|<f>\n"
		    "  gap +5|-5|<px> | bar toggle|show|hide\n"
		    "  traybar toggle|show|hide\n"
		    "  win close | win tag <name> | win toggle float|full|sticky\n"
		    "  win move <dx> <dy> | win resize <dw> <dh> | win center\n"
		    "  query tags|windows|monitors|layout|state\n"
		    "  reload (re-read Xresources) | quit | version\n"
		    "  completions bash|zsh (mwmc client-side; print shell completion)\n");
		return;
	} else {
		snprintf(reply, rsz, "error: unknown command '%s' (try: mwmc help)", verb);
		return;
	}
	drawbars();
}

/* ---- manage / unmanage ---------------------------------------------- */

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;
	Monitor *m;

	c = ecalloc(1, sizeof(Client));
	c->win = w;
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;

	updatetitle(c);
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->tag = t->tag;
		c->isfloating = 1;
	} else {
		applyrules(c);
	}

	m = clientmon(c);
	if (!m)
		m = selmon;

	if (c->x + WIDTH(c) > m->mx + m->mw)
		c->x = m->mx + m->mw - WIDTH(c);
	if (c->y + HEIGHT(c) > m->my + m->mh)
		c->y = m->my + m->mh - HEIGHT(c);
	c->x = MAX(c->x, m->mx);
	c->y = MAX(c->y, m->my);
	c->bw = borderpx;

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, (scheme[SchemeNorm][ColBorder].pixel & 0x00ffffff) | 0xff000000UL);
	configure(c);
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);

	/* center floating windows */
	if (c->isfloating) {
		c->x = m->wx + (m->ww - WIDTH(c)) / 2;
		c->y = m->wy + (m->wh - HEIGHT(c)) / 2;
	}

	XSelectInput(dpy, w, EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask);
	grabbuttons(c, 0);
	XRaiseWindow(dpy, c->win);
	attach(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
	                (unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h);
	setclientstate(c, NormalState);
	if (clientmon(c) == selmon)
		unfocus(selmon->sel, 0);
	if (clientmon(c))
		clientmon(c)->sel = c;
	arrange(clientmon(c));
	XMapWindow(dpy, c->win);
	focus(NULL);
}

void
unmanage(Client *c, int destroyed)
{
	Monitor *m = clientmon(c);
	XWindowChanges wc;

	detach(c);
	detachstack(c);
	if (!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSelectInput(dpy, c->win, NoEventMask);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);
	focus(NULL);
	updateclientlist();
	arrange(m);
}

/* ---- properties ------------------------------------------------------ */

void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if (c->name[0] == '\0')
		strcpy(c->name, broken);
}

void
updatewindowtype(Client *c)
{
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
}

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
	}
}

void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
	c->hintsvalid = 1;
}

void
updatestatus(void)
{
	if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
		strcpy(stext, "mwm-"VERSION);
	drawbars();
}

void
updateclientlist(void)
{
	Client *c;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (c = clients; c; c = c->next)
		XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32,
		                PropModeAppend, (unsigned char *) &(c->win), 1);
}

/* ---- system tray ----------------------------------------------------- */

unsigned int
getsystraywidth(void)
{
	unsigned int w = 0;
	int n = 0;
	Client *i;

	if (!showsystray || !systray || !systrayvisible)
		return 0;
	for (i = systray->icons; i; i = i->next) {
		w += i->w + systrayspacing;
		n++;
	}
	if (n == 0)
		return 0;
	/* icon block + the pill's left/right padding (same as the bar segments) */
	return w - systrayspacing + 2 * (lrpad / 2);
}

void
removesystrayicon(Client *i)
{
	Client **ii;

	if (!showsystray || !i)
		return;
	for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next)
		;
	if (*ii)
		*ii = i->next;
	free(i);
}

void
updatesystrayicongeom(Client *i, int w, int h)
{
	int ih = bh - 2 * systrayvertpad;

	if (!i)
		return;
	i->h = ih;
	if (w == h)
		i->w = ih;
	else if (h == ih)
		i->w = w;
	else
		i->w = (int)((float)ih * ((float)w / (float)h));
	if (i->w <= 0)
		i->w = ih;
}

void
updatesystrayiconstate(Client *i, XPropertyEvent *ev)
{
	long flags;
	int code = 0;
	Atom type;
	int fmt;
	unsigned long nitems, after;
	unsigned char *data = NULL;

	if (!showsystray || !i || ev->atom != xatom[XembedInfo] ||
	    XGetWindowProperty(dpy, i->win, xatom[XembedInfo], 0, 2, False, xatom[XembedInfo],
	                       &type, &fmt, &nitems, &after, &data) != Success)
		return;
	if (!data || nitems < 2) {
		if (data) XFree(data);
		return;
	}
	flags = ((long *)data)[1];
	if (flags & XEMBED_MAPPED) {
		code = XEMBED_WINDOW_ACTIVATE;
		XMapRaised(dpy, i->win);
		setclientstate(i, NormalState);
	} else {
		code = XEMBED_WINDOW_ACTIVATE; /* keep activated */
		XMapRaised(dpy, i->win);
	}
	sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0, systray->win, XEMBED_EMBEDDED_VERSION);
	XFree(data);
}

void
updatesystray(void)
{
	XSetWindowAttributes wa;
	Client *i;
	Monitor *m = NULL;
	unsigned int x, w;

	if (!showsystray)
		return;
	for (m = mons; m && m->num != systraymon; m = m->next)
		;
	if (!m)
		m = mons;
	if (!m)
		return;

	/* Opaque tray background in the DEFAULT (24-bit) visual. The systray uses
	 * the default visual (not the bar's 32-bit ARGB one) so legacy GTK tray
	 * apps create 24-bit icons that inherit this background via ParentRelative
	 * instead of painting their own opaque theme background. 24-bit has no
	 * alpha, so the pill is opaque (the SchemeStatus base colour). */
	unsigned long traybg = ((unsigned long)(scheme[SchemeStatus][ColBg].color.red   >> 8) << 16)
	                     | ((unsigned long)(scheme[SchemeStatus][ColBg].color.green >> 8) <<  8)
	                     |  (unsigned long)(scheme[SchemeStatus][ColBg].color.blue  >> 8);

	if (!systray) {
		systray = ecalloc(1, sizeof(Systray));
		wa.override_redirect = True;
		wa.event_mask = ButtonPressMask | ExposureMask;
		wa.background_pixel = traybg;
		wa.border_pixel = 0;
		systray->win = XCreateWindow(dpy, root, m->mx + m->mw, m->by, 1, bh, 0,
		                             DefaultDepth(dpy, screen), InputOutput,
		                             DefaultVisual(dpy, screen),
		                             CWOverrideRedirect | CWEventMask | CWBackPixel | CWBorderPixel, &wa);
		XSelectInput(dpy, systray->win, SubstructureNotifyMask | ButtonPressMask | ExposureMask);
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
		                PropModeReplace, (unsigned char *)&netatom[NetSystemTrayOrientationHorz], 1);
		{
			long v = XVisualIDFromVisual(DefaultVisual(dpy, screen));
			XChangeProperty(dpy, systray->win, netatom[NetSystemTrayVisual], XA_VISUALID, 32,
			                PropModeReplace, (unsigned char *)&v, 1);
		}
		XChangeProperty(dpy, systray->win, netatom[NetWMWindowType], XA_ATOM, 32,
		                PropModeReplace, (unsigned char *)&netatom[NetWMWindowTypeDock], 1);
		XMapRaised(dpy, systray->win);
		XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
		if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
			sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
			XSync(dpy, False);
		} else {
			fprintf(stderr, "mwm: unable to obtain system tray selection.\n");
			free(systray);
			systray = NULL;
			return;
		}
	}

	/* tray hidden via `mwmc traybar toggle`: unmap ONLY the container window.
	 * The icons are reparented children of it, so they vanish with it but stay
	 * mapped in their own right — do NOT unmap them individually, or the
	 * resulting UnmapNotify makes unmapnotify() call removesystrayicon() and the
	 * icons are gone for good (they never reappear on show). We keep owning the
	 * tray selection while hidden so new icons still dock. */
	if (!systrayvisible) {
		XUnmapWindow(dpy, systray->win);
		return;
	}

	/* re-map the tray window: it is only mapped once at creation, so after a
	 * `traybar hide` (which unmaps it) we must map it again to show it. */
	XMapRaised(dpy, systray->win);

	int pad = lrpad / 2; /* match the bar pills' left/right padding */

	w = 0;
	for (i = systray->icons; i; i = i->next) {
		XSetWindowBackgroundPixmap(dpy, i->win, ParentRelative);
		XMapRaised(dpy, i->win);
		XMoveResizeWindow(dpy, i->win, pad + w, systrayvertpad, i->w, i->h);
		w += i->w + systrayspacing;
	}
	w = w ? w - systrayspacing : 0;
	if (w == 0) {
		/* nothing docked: park the tray window off to the side */
		XMoveResizeWindow(dpy, systray->win, m->mx + m->mw, m->by, 1, bh);
	} else {
		unsigned int tw = w + 2 * pad;
		x = m->mx + m->mw - barmargin - tw;
		XSetWindowBackground(dpy, systray->win, traybg);
		XMoveResizeWindow(dpy, systray->win, x, m->by, tw, bh);
		XClearWindow(dpy, systray->win);
		if (roundcorners)
			roundwin(systray->win, tw, bh, segradius, 0);
		else
			clearround(systray->win);
	}
	XSync(dpy, False);
}

/* ---- misc X helpers -------------------------------------------------- */

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
	                PropModeReplace, (unsigned char *)data, 2);
}

int
sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4)
{
	int n;
	Atom *protocols, mt;
	int exists = 0;
	XEvent ev;

	if (proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
		mt = wmatom[WMProtocols];
		if (XGetWMProtocols(dpy, w, &protocols, &n)) {
			while (!exists && n--)
				exists = protocols[n] == proto;
			XFree(protocols);
		}
	} else {
		exists = 1;
		mt = proto;
	}
	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = w;
		ev.xclient.message_type = mt;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = d0;
		ev.xclient.data.l[1] = d1;
		ev.xclient.data.l[2] = d2;
		ev.xclient.data.l[3] = d3;
		ev.xclient.data.l[4] = d4;
		XSendEvent(dpy, w, False, mask, &ev);
	}
	return exists;
}

Atom
getatomprop(Client *c, Atom prop)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
	                       &da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		XFree(p);
	}
	return atom;
}

int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
	                       &real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING) {
		strncpy(text, (char *)name.value, size - 1);
	} else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
		strncpy(text, *list, size - 1);
		XFreeStringList(list);
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

Client *
wintoclient(Window w)
{
	Client *c;

	for (c = clients; c; c = c->next)
		if (c->win == w)
			return c;
	return NULL;
}

Client *
wintosystrayicon(Window w)
{
	Client *i = NULL;

	if (!showsystray || !systray || !w)
		return i;
	for (i = systray->icons; i && i->win != w; i = i->next)
		;
	return i;
}

Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	for (m = mons; m; m = m->next)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for (m = mons; m; m = m->next)
		if (w == m->barwin)
			return m;
	if ((c = wintoclient(w)))
		return clientmon(c) ? clientmon(c) : selmon;
	return selmon;
}

/* ---- monitors / geometry -------------------------------------------- */

Monitor *
createmon(int num)
{
	Monitor *m = ecalloc(1, sizeof(Monitor));
	m->num = num;
	m->seltag = -1;
	m->sel = NULL;
	strncpy(m->ltsymbol, layoutsym[defaultlayout], sizeof(m->ltsymbol) - 1);
	return m;
}

#ifdef XINERAMA
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}
#endif /* XINERAMA */

int
updategeom(int initial)
{
	int dirty = 0;
	int oldcount = 0, nn = 1, i;
	Monitor *m;
	int oldseltag[64];
	struct { int x, y, w, h; } geom[64];

	for (m = mons; m; m = m->next)
		oldcount++;

#ifdef XINERAMA
	if (XineramaIsActive(dpy)) {
		int j, k;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		for (j = 0, k = 0; j < nn; j++)
			if (isuniquegeom(unique, k, &info[j]))
				memcpy(&unique[k++], &info[j], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = k;
		if (nn > 64) nn = 64;
		for (j = 0; j < nn; j++) {
			geom[j].x = unique[j].x_org;
			geom[j].y = unique[j].y_org;
			geom[j].w = unique[j].width;
			geom[j].h = unique[j].height;
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{
		nn = 1;
		geom[0].x = 0;
		geom[0].y = 0;
		geom[0].w = sw;
		geom[0].h = sh;
	}

	if (nn != oldcount)
		dirty = 1;
	else {
		i = 0;
		for (m = mons; m && i < nn; m = m->next, i++)
			if (m->mx != geom[i].x || m->my != geom[i].y ||
			    m->mw != geom[i].w || m->mh != geom[i].h)
				dirty = 1;
	}
	if (!dirty)
		return 0;

	/* snapshot old per-monitor selected tags by index */
	for (i = 0; i < 64; i++)
		oldseltag[i] = -1;
	for (i = 0, m = mons; m && i < 64; m = m->next, i++)
		oldseltag[i] = m->seltag;

	/* tear down old monitors (clients are global, so they survive) */
	while (mons) {
		m = mons;
		mons = mons->next;
		if (m->barwin) {
			XUnmapWindow(dpy, m->barwin);
			XDestroyWindow(dpy, m->barwin);
		}
		free(m);
	}

	/* build new monitors */
	mons = NULL;
	{
		Monitor **tail = &mons;
		for (i = 0; i < nn; i++) {
			m = createmon(i);
			m->mx = m->wx = geom[i].x;
			m->my = m->wy = geom[i].y;
			m->mw = m->ww = geom[i].w;
			m->mh = m->wh = geom[i].h;
			if (i < oldcount)
				m->seltag = oldseltag[i];
			*tail = m;
			tail = &m->next;
		}
	}

	/* reassign tags whose owner monitor disappeared */
	for (i = 0; i < MAX_TAGS; i++)
		if (tagstate[i].mon >= nn)
			tagstate[i].mon = nn - 1;

	/* apply default tag->monitor rules for newly appeared monitors */
	for (i = 0; i < (int)LENGTH(tagrules); i++) {
		const TagRule *tr = &tagrules[i];
		if (tr->tag < 0 || tr->tag >= MAX_TAGS || tr->mon < 0 || tr->mon >= nn)
			continue;
		if (tr->mon >= oldcount || initial) {
			if (tagstate[tr->tag].mon < 0)
				tagstate[tr->tag].mon = tr->mon;
		}
	}

	/* ensure each monitor shows one of its owned tags if it has any */
	for (m = mons; m; m = m->next) {
		if (m->seltag >= 0 && tagstate[m->seltag].mon == m->num)
			continue;
		m->seltag = -1;
		for (i = 0; i < MAX_TAGS; i++)
			if (tagstate[i].mon == m->num) {
				m->seltag = i;
				break;
			}
	}

	updatebars();
	selmon = mons;
	return 1;
}

static void
updatebars(void)
{
	Monitor *m;
	XSetWindowAttributes wa;
	XClassHint ch = { "mwm", "mwm" };

	wa.override_redirect = True;
	wa.background_pixel = 0;
	wa.border_pixel = 0;
	wa.colormap = cmap;
	wa.event_mask = ButtonPressMask | ExposureMask;

	for (m = mons; m; m = m->next) {
		updatebarpos(m);
		if (!m->barwin) {
			m->barwin = XCreateWindow(dpy, root, m->mx, m->by, m->mw, bh, 0, depth,
			                          InputOutput, visual,
			                          CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWColormap | CWEventMask, &wa);
			XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
			XMapRaised(dpy, m->barwin);
			XSetClassHint(dpy, m->barwin, &ch);
		} else
			XMoveResizeWindow(dpy, m->barwin, m->mx, m->by, m->mw, bh);
	}
}

/* ---- startup / shutdown --------------------------------------------- */

void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
}

void
sigchld(int unused)
{
	(void)unused;
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		die("can't install SIGCHLD handler:");
	while (0 < waitpid(-1, NULL, WNOHANG))
		;
}

int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "mwm: fatal error: request code=%d, error code=%d\n",
	        ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee);
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	(void)dpy; (void)ee;
	return 0;
}

int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	(void)dpy; (void)ee;
	die("mwm: another window manager is already running");
	return -1;
}

void
checkotherwm(void)
{
	xerrorxlib = XSetErrorHandler(xerrorstart);
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
resource_load(XrmDatabase db, const char *name, ResType type, void *dst)
{
	char fullname[256], fullclass[256], *xtype = NULL;
	XrmValue ret;

	snprintf(fullname, sizeof fullname, "mwm.%s", name);
	snprintf(fullclass, sizeof fullclass, "Mwm.%s", name);
	if (!XrmGetResource(db, fullname, fullclass, &xtype, &ret)
	|| !ret.addr || !xtype || strncmp("String", xtype, 64))
		return;
	switch (type) {
	case STRING_RES:
		*(char **)dst = strdup(ret.addr); /* leaks the compiled default; fine, one-shot */
		break;
	case INTEGER_RES:
		*(int *)dst = (int)strtol(ret.addr, NULL, 0); /* base 0 -> accepts 0xNN */
		break;
	case FLOAT_RES:
		*(float *)dst = strtof(ret.addr, NULL);
		break;
	}
}

void
load_xresources(void)
{
	XrmDatabase db;
	unsigned int i;
	Atom rmatom, type;
	int format;
	unsigned long nitems, after;
	unsigned char *data = NULL;

	XrmInitialize();
	/* Read RESOURCE_MANAGER straight off the root window every time. Do NOT use
	 * XResourceManagerString(): it caches the value from XOpenDisplay, so a
	 * later `xrdb -merge` + `mwmc reload` would re-apply the stale startup DB. */
	rmatom = XInternAtom(dpy, "RESOURCE_MANAGER", False);
	if (XGetWindowProperty(dpy, root, rmatom, 0, 256L * 1024L, False, AnyPropertyType,
	                       &type, &format, &nitems, &after, &data) != Success || !data)
		return;
	if ((db = XrmGetStringDatabase((char *)data))) {
		for (i = 0; i < LENGTH(resources); i++)
			resource_load(db, resources[i].name, resources[i].type, resources[i].dst);
		XrmDestroyDatabase(db);
	}
	XFree(data);
}

/* Re-read Xresources and rebuild fonts/colors/geometry live (mwmc reload).
 * Validates colors and fonts first so a typo can't kill the running WM.
 * Returns NULL on success or a short error string. */
const char *
reloadstyle(void)
{
	Monitor *m;
	Client *c;
	Fnt *oldfonts;
	XftColor probe;
	int i, j;

	load_xresources();

	/* validate every color before touching anything */
	for (i = 0; i < SchemeLast; i++)
		for (j = 0; j < 3; j++) {
			if (!XftColorAllocName(dpy, visual, cmap, colors[i][j], &probe))
				return "bad color in Xresources";
			XftColorFree(dpy, visual, cmap, &probe);
		}

	/* fonts: build the new set, keep the old one if it fails */
	oldfonts = drw->fonts;
	drw->fonts = NULL;
	if (!drw_fontset_create(drw, (const char **)fonts, LENGTH(fonts))) {
		drw->fonts = oldfonts;
		return "bad font in Xresources";
	}
	drw_fontset_free(oldfonts);
	lrpad = drw->fonts->h + barpadx;
	bh = barheight > 0 ? barheight : drw->fonts->h + 2 * barvertpad;
	drw_resize(drw, sw, bh);

	/* color schemes + pill fills (mirror setup()) */
	for (i = 0; i < SchemeLast; i++)
		free(scheme[i]);
	for (i = 0; i < SchemeLast; i++) {
		scheme[i] = drw_scm_create(drw, (const char **)colors[i], alphas[i], 3);
		fillcol[i] = scheme[i][ColBg];
		scheme[i][ColBg].pixel &= 0x00ffffffU;
	}

	gappx_run = gappx;

	/* per-client border width + colors */
	for (c = clients; c; c = c->next) {
		if (c->isfullscreen)
			c->oldbw = borderpx;
		else
			c->bw = borderpx;
		XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel & 0x00ffffff);
	}
	/* bar geometry (bh may have changed with the font) */
	for (m = mons; m; m = m->next) {
		if (m->sel)
			XSetWindowBorder(dpy, m->sel->win, scheme[SchemeSel][ColBorder].pixel & 0x00ffffff);
		updatebarpos(m);
		XMoveResizeWindow(dpy, m->barwin, m->mx, m->by, m->mw, bh);
	}

	arrange(NULL);
	drawbars();
	return NULL;
}

void
setup(void)
{
	int i;
	XSetWindowAttributes wa;
	Atom utf8string;
	XVisualInfo vinfo;

	signal(SIGCHLD, sigchld);

	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);

	/* override styling defaults from xrdb / ~/.Xresources before anything
	 * (fonts, colors, geometry) is read */
	load_xresources();

	/* initial bar / tray visibility (live-toggled via mwmc bar|traybar) */
	barvisible = showbar;
	systrayvisible = showtraybar;

	/* prefer a 32-bit ARGB visual so the bar can be translucent */
	if (XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo)) {
		visual = vinfo.visual;
		depth = vinfo.depth;
		cmap = XCreateColormap(dpy, root, visual, AllocNone);
	} else {
		visual = DefaultVisual(dpy, screen);
		depth = DefaultDepth(dpy, screen);
		cmap = DefaultColormap(dpy, screen);
	}

	/* shape extension for rounded corners */
	{
		int err;
		shapeext = XShapeQueryExtension(dpy, &shapeevbase, &err);
	}

	drw = drw_create(dpy, screen, root, sw, sh, visual, depth, cmap);
	if (!drw_fontset_create(drw, (const char **)fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	lrpad = drw->fonts->h + barpadx;
	bh = barheight > 0 ? barheight : drw->fonts->h + 2 * barvertpad;
	gappx_run = gappx;

	/* tag defaults */
	for (i = 0; i < MAX_TAGS; i++) {
		tagstate[i].layout = defaultlayout;
		tagstate[i].nmaster = nmaster;
		tagstate[i].mfact = mfact;
		tagstate[i].mon = -1;
	}

	/* atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	xa_utf8 = utf8string;
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
	netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
	netatom[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
	netatom[NetSystemTrayOrientationHorz] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
	netatom[NetSystemTrayVisual] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_VISUAL", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMStateDemandsAttention] = XInternAtom(dpy, "_NET_WM_STATE_DEMANDS_ATTENTION", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDock] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
	xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
	xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
	netatom[Xembed] = xatom[Xembed]; /* alias used in maprequest/clientmessage */
	mwmatom[MwmMsg] = XInternAtom(dpy, MWM_ATOM_MSG, False);
	mwmatom[MwmCmd] = XInternAtom(dpy, MWM_ATOM_CMD, False);
	mwmatom[MwmReply] = XInternAtom(dpy, MWM_ATOM_REPLY, False);

	/* cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);

	/* color schemes */
	scheme = ecalloc(SchemeLast, sizeof(Clr *));
	fillcol = ecalloc(SchemeLast, sizeof(Clr));
	for (i = 0; i < SchemeLast; i++) {
		scheme[i] = drw_scm_create(drw, (const char **)colors[i], alphas[i], 3);
		/* keep the background (with its alpha) for the pill fills, then make
		 * the scheme's own bg transparent so drw_text never paints over the
		 * rounded pill corners */
		fillcol[i] = scheme[i][ColBg];
		scheme[i][ColBg].pixel &= 0x00ffffffU;
	}

	/* supporting wm check window (EWMH) */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
	                PropModeReplace, (unsigned char *)&wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
	                PropModeReplace, (unsigned char *)"mwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
	                PropModeReplace, (unsigned char *)&wmcheckwin, 1);
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
	                PropModeReplace, (unsigned char *)netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);

	updategeom(1);
	if (showsystray)
		updatesystray();

	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
	              | ButtonPressMask | PointerMotionMask | EnterWindowMask
	              | LeaveWindowMask | StructureNotifyMask | PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);

	updatestatus();
	focus(NULL);
}

void
cleanup(void)
{
	Monitor *m;

	running = 0;
	while (clients)
		unmanage(clients, 0);
	if (systray) {
		XUnmapWindow(dpy, systray->win);
		XDestroyWindow(dpy, systray->win);
		free(systray);
	}
	while (mons) {
		m = mons;
		mons = mons->next;
		if (m->barwin) {
			XUnmapWindow(dpy, m->barwin);
			XDestroyWindow(dpy, m->barwin);
		}
		free(m);
	}
	drw_cur_free(drw, cursor[CurNormal]);
	drw_cur_free(drw, cursor[CurResize]);
	drw_cur_free(drw, cursor[CurMove]);
	{
		int i;
		for (i = 0; i < SchemeLast; i++)
			free(scheme[i]);
	}
	free(scheme);
	free(fillcol);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void
run(void)
{
	XEvent ev;

	XSync(dpy, False);
	while (running && !XNextEvent(dpy, &ev))
		if (handler[ev.type])
			handler[ev.type](&ev);
}

int
main(int argc, char *argv[])
{
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("mwm-"VERSION);
	else if (argc != 1)
		die("usage: mwm [-v]");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("mwm: cannot open display");
	checkotherwm();
	setup();
	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
