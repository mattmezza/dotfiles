/* See LICENSE file for license details. */
#define _XOPEN_SOURCE 500
#if HAVE_SHADOW_H
#include <shadow.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/types.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xinerama.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "arg.h"
#include "util.h"

char *argv0;

/* global count to prevent repeated error messages */
int count_error = 0;

enum {
	INIT,
	INPUT,
	INPUT_ALT,
	FAILED,
	NUMCOLS
};

struct lock {
	int screen;
	Window root, win;
	Pixmap pmap;
	unsigned long colors[NUMCOLS];
};

struct xrandr {
	int active;
	int evbase;
	int errbase;
};

#include "config.h"

static void
die(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(1);
}

#ifdef __linux__
#include <fcntl.h>
#include <linux/oom.h>

static void
dontkillme(void)
{
	FILE *f;
	const char oomfile[] = "/proc/self/oom_score_adj";

	if (!(f = fopen(oomfile, "w"))) {
		if (errno == ENOENT)
			return;
		die("slock: fopen %s: %s\n", oomfile, strerror(errno));
	}
	fprintf(f, "%d", OOM_SCORE_ADJ_MIN);
	if (fclose(f)) {
		if (errno == EACCES)
			die("slock: unable to disable OOM killer. "
			    "Make sure to suid or sgid slock.\n");
		else
			die("slock: fclose %s: %s\n", oomfile, strerror(errno));
	}
}
#endif

static int
readescapedint(const char *str, int *i) {
	int n = 0;
	if (str[*i])
		++*i;
	while(str[*i] && str[*i] != ';' && str[*i] != 'm') {
		n = 10 * n + str[*i] - '0';
		++*i;
	}
	return n;
}

static void
writemessage(Display *dpy, Window win, int screen)
{
	int len, line_len, width, height, s_width, s_height, i, k, tab_size, r, g, b, escaped_int, curr_line_len;
	XGCValues gr_values;
	XFontStruct *fontinfo;
	XColor color, dummy;
	XineramaScreenInfo *xsi;
	GC gc;
	fontinfo = XLoadQueryFont(dpy, font_name);

	if (fontinfo == NULL) {
		if (count_error == 0) {
			fprintf(stderr, "slock: Unable to load font \"%s\"\n", font_name);
			fprintf(stderr, "slock: Try listing fonts with 'slock -f'\n");
			count_error++;
		}
		return;
	}

	tab_size = 8 * XTextWidth(fontinfo, " ", 1);

	XAllocNamedColor(dpy, DefaultColormap(dpy, screen),
		 text_color, &color, &dummy);

	gr_values.font = fontinfo->fid;
	gr_values.foreground = color.pixel;
	gc=XCreateGC(dpy,win,GCFont+GCForeground, &gr_values);

	/*  To prevent "Uninitialized" warnings. */
	xsi = NULL;

	/*
	 * Start formatting and drawing text
	 */

	len = strlen(message);

	/* Max max line length (cut at '\n') */
	line_len = curr_line_len = 0;
	k = 0;
	for (i = 0; i < len; i++) {
		if (message[i] == '\n') {
			curr_line_len = 0;
			k++;
		} else if (message[i] == 0x1b) {
			while (i < len && message[i] != 'm') {
				i++;
			}
			if (i == len)
				die("slock: unclosed escape sequence\n");
		} else {
			curr_line_len += XTextWidth(fontinfo, message + i, 1);
			if (curr_line_len > line_len)
				line_len = curr_line_len;
		}
	}
	/* If there is only one line */
	if (line_len == 0)
		line_len = len;

	if (XineramaIsActive(dpy)) {
		xsi = XineramaQueryScreens(dpy, &i);
		s_width = xsi[0].width;
		s_height = xsi[0].height;
	} else {
		s_width = DisplayWidth(dpy, screen);
		s_height = DisplayHeight(dpy, screen);
	}
	height = s_height*3/7 - (k*20)/3;
	width  = (s_width - line_len)/2;

	line_len = 0;
	/* print the text while parsing 24 bit color ANSI escape codes*/
	for (i = k = 0; i < len; i++) {
		switch (message[i]) {
			case '\n':
				line_len = 0;
				while (message[i + 1] == '\t') {
					line_len += tab_size;
					i++;
				}
				k++;
				break;
			case 0x1b:
				i++;
				if (message[i] == '[') {
					escaped_int = readescapedint(message, &i);
					if (escaped_int == 39)
						continue;
					if (escaped_int != 38)
						die("slock: unknown escape sequence%d\n", escaped_int);
					if (readescapedint(message, &i) != 2)
						die("slock: only 24 bit color supported\n");
					r = readescapedint(message, &i) & 0xff;
					g = readescapedint(message, &i) & 0xff;
					b = readescapedint(message, &i) & 0xff;
					XSetForeground(dpy, gc, r << 16 | g << 8 | b);
				} else
					die("slock: unknown escape sequence\n");
				break;
			default:
				XDrawString(dpy, win, gc, width + line_len, height + 20 * k, message + i, 1);
				line_len += XTextWidth(fontinfo, message + i, 1);
		}
	}

	/* xsi should not be NULL anyway if Xinerama is active, but to be safe */
	if (XineramaIsActive(dpy) && xsi != NULL)
			XFree(xsi);
}



static const char *
gethash(void)
{
	const char *hash;
	struct passwd *pw;

	/* Check if the current user has a password entry */
	errno = 0;
	if (!(pw = getpwuid(getuid()))) {
		if (errno)
			die("slock: getpwuid: %s\n", strerror(errno));
		else
			die("slock: cannot retrieve password entry\n");
	}
	hash = pw->pw_passwd;

#if HAVE_SHADOW_H
	if (!strcmp(hash, "x")) {
		struct spwd *sp;
		if (!(sp = getspnam(pw->pw_name)))
			die("slock: getspnam: cannot retrieve shadow entry. "
			    "Make sure to suid or sgid slock.\n");
		hash = sp->sp_pwdp;
	}
#else
	if (!strcmp(hash, "*")) {
#ifdef __OpenBSD__
		if (!(pw = getpwuid_shadow(getuid())))
			die("slock: getpwnam_shadow: cannot retrieve shadow entry. "
			    "Make sure to suid or sgid slock.\n");
		hash = pw->pw_passwd;
#else
		die("slock: getpwuid: cannot retrieve shadow entry. "
		    "Make sure to suid or sgid slock.\n");
#endif /* __OpenBSD__ */
	}
#endif /* HAVE_SHADOW_H */

	return hash;
}

static void
readpw(Display *dpy, struct xrandr *rr, struct lock **locks, int nscreens,
       const char *hash)
{
	XRRScreenChangeNotifyEvent *rre;
	char buf[32], passwd[256], *inputhash;
	int num, screen, running, failure, oldc;
	unsigned int len, color;
	KeySym ksym;
	XEvent ev;

	len = 0;
	running = 1;
	failure = 0;
	oldc = INIT;

	while (running && !XNextEvent(dpy, &ev)) {
		if (ev.type == KeyPress) {
			explicit_bzero(&buf, sizeof(buf));
			num = XLookupString(&ev.xkey, buf, sizeof(buf), &ksym, 0);
			if (IsKeypadKey(ksym)) {
				if (ksym == XK_KP_Enter)
					ksym = XK_Return;
				else if (ksym >= XK_KP_0 && ksym <= XK_KP_9)
					ksym = (ksym - XK_KP_0) + XK_0;
			}
			if (IsFunctionKey(ksym) ||
			    IsKeypadKey(ksym) ||
			    IsMiscFunctionKey(ksym) ||
			    IsPFKey(ksym) ||
			    IsPrivateKeypadKey(ksym))
				continue;
			switch (ksym) {
			case XK_Return:
				passwd[len] = '\0';
				errno = 0;
				if (!(inputhash = crypt(passwd, hash)))
					fprintf(stderr, "slock: crypt: %s\n", strerror(errno));
				else
					running = !!strcmp(inputhash, hash);
				if (running) {
					XBell(dpy, 100);
					failure = 1;
				}
				explicit_bzero(&passwd, sizeof(passwd));
				len = 0;
				break;
			case XK_Escape:
				explicit_bzero(&passwd, sizeof(passwd));
				len = 0;
				break;
			case XK_BackSpace:
				if (len)
					passwd[--len] = '\0';
				break;
			default:
				if (num && !iscntrl((int)buf[0]) &&
				    (len + num < sizeof(passwd))) {
					memcpy(passwd + len, buf, num);
					len += num;
				}
				break;
			}
			color = len ? (len%2 ? INPUT : INPUT_ALT)
			            : ((failure || failonclear) ? FAILED : INIT);
			if (running && oldc != color) {
				for (screen = 0; screen < nscreens; screen++) {
					XSetWindowBackground(dpy,
					                     locks[screen]->win,
					                     locks[screen]->colors[color]);
					XClearWindow(dpy, locks[screen]->win);
					writemessage(dpy, locks[screen]->win, screen);
				}
				oldc = color;
			}
		} else if (rr->active && ev.type == rr->evbase + RRScreenChangeNotify) {
			rre = (XRRScreenChangeNotifyEvent*)&ev;
			for (screen = 0; screen < nscreens; screen++) {
				if (locks[screen]->win == rre->window) {
					if (rre->rotation == RR_Rotate_90 ||
					    rre->rotation == RR_Rotate_270)
						XResizeWindow(dpy, locks[screen]->win,
						              rre->height, rre->width);
					else
						XResizeWindow(dpy, locks[screen]->win,
						              rre->width, rre->height);
					XClearWindow(dpy, locks[screen]->win);
					break;
				}
			}
		} else {
			for (screen = 0; screen < nscreens; screen++)
				XRaiseWindow(dpy, locks[screen]->win);
		}
	}
}

static struct lock *
lockscreen(Display *dpy, struct xrandr *rr, int screen)
{
	char curs[] = {0, 0, 0, 0, 0, 0, 0, 0};
	int i, ptgrab, kbgrab;
	struct lock *lock;
	XColor color, dummy;
	XSetWindowAttributes wa;
	Cursor invisible;

	if (dpy == NULL || screen < 0 || !(lock = malloc(sizeof(struct lock))))
		return NULL;

	lock->screen = screen;
	lock->root = RootWindow(dpy, lock->screen);

	for (i = 0; i < NUMCOLS; i++) {
		XAllocNamedColor(dpy, DefaultColormap(dpy, lock->screen),
		                 colorname[i], &color, &dummy);
		lock->colors[i] = color.pixel;
	}

	/* init */
	wa.override_redirect = 1;
	wa.background_pixel = lock->colors[INIT];
	lock->win = XCreateWindow(dpy, lock->root, 0, 0,
	                          DisplayWidth(dpy, lock->screen),
	                          DisplayHeight(dpy, lock->screen),
	                          0, DefaultDepth(dpy, lock->screen),
	                          CopyFromParent,
	                          DefaultVisual(dpy, lock->screen),
	                          CWOverrideRedirect | CWBackPixel, &wa);
	lock->pmap = XCreateBitmapFromData(dpy, lock->win, curs, 8, 8);
	invisible = XCreatePixmapCursor(dpy, lock->pmap, lock->pmap,
	                                &color, &color, 0, 0);
	XDefineCursor(dpy, lock->win, invisible);

	/* Try to grab mouse pointer *and* keyboard for 600ms, else fail the lock */
	for (i = 0, ptgrab = kbgrab = -1; i < 6; i++) {
		if (ptgrab != GrabSuccess) {
			ptgrab = XGrabPointer(dpy, lock->root, False,
			                      ButtonPressMask | ButtonReleaseMask |
			                      PointerMotionMask, GrabModeAsync,
			                      GrabModeAsync, None, invisible, CurrentTime);
		}
		if (kbgrab != GrabSuccess) {
			kbgrab = XGrabKeyboard(dpy, lock->root, True,
			                       GrabModeAsync, GrabModeAsync, CurrentTime);
		}

		/* input is grabbed: we can lock the screen */
		if (ptgrab == GrabSuccess && kbgrab == GrabSuccess) {
			XMapRaised(dpy, lock->win);
			if (rr->active)
				XRRSelectInput(dpy, lock->win, RRScreenChangeNotifyMask);

			XSelectInput(dpy, lock->root, SubstructureNotifyMask);
			return lock;
		}

		/* retry on AlreadyGrabbed but fail on other errors */
		if ((ptgrab != AlreadyGrabbed && ptgrab != GrabSuccess) ||
		    (kbgrab != AlreadyGrabbed && kbgrab != GrabSuccess))
			break;

		usleep(100000);
	}

	/* we couldn't grab all input: fail out */
	if (ptgrab != GrabSuccess)
		fprintf(stderr, "slock: unable to grab mouse pointer for screen %d\n",
		        screen);
	if (kbgrab != GrabSuccess)
		fprintf(stderr, "slock: unable to grab keyboard for screen %d\n",
		        screen);
	return NULL;
}

static void
usage(void)
{
	die("usage: slock [-v] [-f] [-m message] [cmd [arg ...]]\n");
}

int
main(int argc, char **argv) {
	struct xrandr rr;
	struct lock **locks;
	struct passwd *pwd;
	struct group *grp;
	uid_t duid;
	gid_t dgid;
	const char *hash;
	Display *dpy;
	int i, s, nlocks, nscreens;
	int count_fonts;
	char **font_names;

	ARGBEGIN {
	case 'v':
		puts("slock-"VERSION);
		return 0;
	case 'm':
		message = EARGF(usage());
		break;
	case 'f':
		if (!(dpy = XOpenDisplay(NULL)))
			die("slock: cannot open display\n");
		font_names = XListFonts(dpy, "*", 10000 /* list 10000 fonts*/, &count_fonts);
		for (i=0; i<count_fonts; i++) {
			fprintf(stderr, "%s\n", *(font_names+i));
		}
		return 0;
	default:
		usage();
	} ARGEND

	/* validate drop-user and -group */
	errno = 0;
	if (!(pwd = getpwnam(user)))
		die("slock: getpwnam %s: %s\n", user,
		    errno ? strerror(errno) : "user entry not found");
	duid = pwd->pw_uid;
	errno = 0;
	if (!(grp = getgrnam(group)))
		die("slock: getgrnam %s: %s\n", group,
		    errno ? strerror(errno) : "group entry not found");
	dgid = grp->gr_gid;

#ifdef __linux__
	dontkillme();
#endif

	hash = gethash();
	errno = 0;
	if (!crypt("", hash))
		die("slock: crypt: %s\n", strerror(errno));

	if (!(dpy = XOpenDisplay(NULL)))
		die("slock: cannot open display\n");

	/* drop privileges */
	if (setgroups(0, NULL) < 0)
		die("slock: setgroups: %s\n", strerror(errno));
	if (setgid(dgid) < 0)
		die("slock: setgid: %s\n", strerror(errno));
	if (setuid(duid) < 0)
		die("slock: setuid: %s\n", strerror(errno));

	/* check for Xrandr support */
	rr.active = XRRQueryExtension(dpy, &rr.evbase, &rr.errbase);

	/* get number of screens in display "dpy" and blank them */
	nscreens = ScreenCount(dpy);
	if (!(locks = calloc(nscreens, sizeof(struct lock *))))
		die("slock: out of memory\n");
	for (nlocks = 0, s = 0; s < nscreens; s++) {
		if ((locks[s] = lockscreen(dpy, &rr, s)) != NULL) {
			writemessage(dpy, locks[s]->win, s);
			nlocks++;
		} else {
			break;
		}
	}
	XSync(dpy, 0);

	/* did we manage to lock everything? */
	if (nlocks != nscreens)
		return 1;

	/* run post-lock command */
	if (argc > 0) {
		pid_t pid;
		extern char **environ;
		int err = posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ);
		if (err) {
			die("slock: failed to execute post-lock command: %s: %s\n",
			    argv[0], strerror(err));
		}
	}

	/* everything is now blank. Wait for the correct password */
	readpw(dpy, &rr, locks, nscreens, hash);

	return 0;
}
