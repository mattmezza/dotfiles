/* mwmc - the mwm control client.
 *
 * Joins argv into a NUL-separated blob, stashes it on a throwaway window, pokes
 * mwm with a ClientMessage, then blocks (with a timeout) for the reply mwm
 * writes back onto that window. See ipc.h for the protocol. */
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "ipc.h"
#include "completions.h"   /* generated: completion_bash[], completion_zsh[] */

#define REPLY_TIMEOUT_MS 2000

static void
usage(void)
{
	fputs(
	"usage: mwmc <command> [args...]   (window <= tag <= monitor)\n"
	"\n"
	"  focus next|prev|last            focus next/previous/last-used window\n"
	"  focus win next|prev|last|<class> focus window (direction, last, or WM_CLASS)\n"
	"  focus tag next|prev|<name>      view a tag\n"
	"  focus mon next|prev|<n>         focus a monitor\n"
	"  move win next|prev|<tag>        send focused window to a tag\n"
	"  move win master                 promote focused window to master\n"
	"  move win mon next|prev          send focused window to another monitor\n"
	"  move tag next|prev|<mon>        move current tag (+windows) to a monitor\n"
	"  swap next|prev                  reorder focused window in the stack\n"
	"  layout cols|stack|float|next|prev\n"
	"  master +1|-1|<n>                adjust master count\n"
	"  master ratio +0.05|-0.05|<f>    adjust master width\n"
	"  gap +5|-5|<px>                  adjust gaps\n"
	"  bar toggle|show|hide            show/hide the bar\n"
	"  traybar toggle|show|hide        show/hide the system tray\n"
	"  win close                       close focused window\n"
	"  win tag <name>                  move focused window to a tag\n"
	"  win toggle float|full|sticky\n"
	"  win move <dx> <dy>              nudge a floating window (auto-floats)\n"
	"  win resize <dw> <dh>            grow/shrink a floating window\n"
	"  win center                      center a floating window\n"
	"  (mouse: hold Super and drag = move, Super + right-drag = resize)\n"
	"  query tags|windows|monitors|layout|state\n"
	"  reload                          re-read Xresources (xrdb) and restyle live\n"
	"  completions bash|zsh            print a shell completion script (redirect it,\n"
	"                                  e.g. mwmc completions bash > \\\n"
	"                                  ~/.local/share/bash-completion/completions/mwmc)\n"
	"  quit | version | help\n",
	stderr);
}

int
main(int argc, char *argv[])
{
	Display *dpy;
	Window root, w;
	Atom a_msg, a_cmd, a_reply, a_utf8;
	XEvent ev;
	char *buf;
	size_t len = 0, off = 0;
	int i, rc = 0, fd;

	if (argc < 2) {
		usage();
		return 2;
	}
	if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		usage();
		return 0;
	}

	/* `completions bash|zsh` prints an embedded completion script to stdout and
	 * exits, without needing a running mwm (handled before opening a display) */
	if (!strcmp(argv[1], "completions")) {
		if (argc > 2 && !strcmp(argv[2], "bash")) { fputs(completion_bash, stdout); return 0; }
		if (argc > 2 && !strcmp(argv[2], "zsh"))  { fputs(completion_zsh, stdout);  return 0; }
		fputs("usage: mwmc completions bash|zsh\n", stderr);
		return 2;
	}

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "mwmc: cannot open display\n");
		return 1;
	}
	root = DefaultRootWindow(dpy);
	a_msg   = XInternAtom(dpy, MWM_ATOM_MSG, False);
	a_cmd   = XInternAtom(dpy, MWM_ATOM_CMD, False);
	a_reply = XInternAtom(dpy, MWM_ATOM_REPLY, False);
	a_utf8  = XInternAtom(dpy, "UTF8_STRING", False);

	/* build NUL-separated argv blob */
	for (i = 1; i < argc; i++)
		len += strlen(argv[i]) + 1;
	if (!(buf = malloc(len))) {
		fprintf(stderr, "mwmc: out of memory\n");
		XCloseDisplay(dpy);
		return 1;
	}
	for (i = 1; i < argc; i++) {
		size_t l = strlen(argv[i]) + 1;
		memcpy(buf + off, argv[i], l);
		off += l;
	}

	/* throwaway requestor window we listen on for the reply */
	w = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XSelectInput(dpy, w, PropertyChangeMask);
	XChangeProperty(dpy, w, a_cmd, a_utf8, 8, PropModeReplace,
	                (unsigned char *)buf, len);
	free(buf);

	memset(&ev, 0, sizeof ev);
	ev.xclient.type = ClientMessage;
	ev.xclient.window = root;
	ev.xclient.message_type = a_msg;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = (long)w;
	XSendEvent(dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
	XFlush(dpy);

	/* wait for mwm to set the reply property on our window (with timeout) */
	fd = ConnectionNumber(dpy);
	for (;;) {
		while (XPending(dpy)) {
			XNextEvent(dpy, &ev);
			if (ev.type == PropertyNotify && ev.xproperty.window == w
			&& ev.xproperty.atom == a_reply && ev.xproperty.state == PropertyNewValue) {
				Atom type;
				int fmt;
				unsigned long nitems, after;
				unsigned char *data = NULL;
				if (XGetWindowProperty(dpy, w, a_reply, 0, (1 << 20), False, AnyPropertyType,
				                       &type, &fmt, &nitems, &after, &data) == Success && data) {
					if (nitems) {
						fwrite(data, 1, nitems, stdout);
						if (((char *)data)[nitems - 1] != '\n')
							fputc('\n', stdout);
						if (!strncmp((char *)data, "error:", 6))
							rc = 1;
					}
					XFree(data);
				}
				goto done;
			}
		}
		{
			struct pollfd pfd = { fd, POLLIN, 0 };
			int pr = poll(&pfd, 1, REPLY_TIMEOUT_MS);
			if (pr == 0) {
				fprintf(stderr, "mwmc: timed out waiting for mwm (is it running?)\n");
				rc = 1;
				goto done;
			}
			if (pr < 0) {
				fprintf(stderr, "mwmc: poll failed\n");
				rc = 1;
				goto done;
			}
		}
	}
done:
	XDestroyWindow(dpy, w);
	XCloseDisplay(dpy);
	return rc;
}
