# lok version
VERSION = 0.1

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

PKG_CONFIG = pkg-config
PKGS = x11 xext xrandr pangocairo

# includes and libs
INCS = `$(PKG_CONFIG) --cflags $(PKGS)`
LIBS = `$(PKG_CONFIG) --libs $(PKGS)` -lcrypt

# flags
CPPFLAGS = -DVERSION=\"$(VERSION)\" -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=700
CFLAGS = -std=c11 -Wall -Wextra -Wno-unused-parameter -Os $(INCS) $(CPPFLAGS)
LDFLAGS = $(LIBS)

# compiler and linker
CC = cc
