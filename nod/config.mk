# nod - suckless notification daemon

VERSION = 0.1

PREFIX    = /usr/local
MANPREFIX = $(PREFIX)/share/man

X11INC = /usr/include/X11
X11LIB = /usr/lib/X11

# Xft
XFTINC = `pkg-config --cflags xft`
XFTLIB = `pkg-config --libs xft`

# freetype (pulled in by xft)
FREETYPEINC = `pkg-config --cflags freetype2`

# D-Bus
DBUSINC = `pkg-config --cflags dbus-1`
DBUSLIB = `pkg-config --libs dbus-1`

INCS = -I$(X11INC) $(XFTINC) $(FREETYPEINC) $(DBUSINC)
LIBS = -L$(X11LIB) -lX11 -lXrandr $(XFTLIB) $(DBUSLIB)

CPPFLAGS = -DVERSION=\"$(VERSION)\" -D_POSIX_C_SOURCE=200809L
CFLAGS   = -Wall -Wextra -pedantic -std=c99 -Os $(INCS) $(CPPFLAGS)
LDFLAGS  = $(LIBS)

CC = cc
