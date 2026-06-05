# mwm version
VERSION = 0.1

# paths
PREFIX    = /usr/local
MANPREFIX = $(PREFIX)/share/man

# Xinerama (multi-monitor). Comment these two lines out to build without it.
XINERAMALIBS  = -lXinerama
XINERAMAFLAGS = -DXINERAMA

# freetype / fontconfig
FREETYPELIBS = -lfontconfig -lXft
FREETYPEINC  = /usr/include/freetype2
# OpenBSD (uncomment)
#FREETYPEINC = $(X11INC)/freetype2

# includes and libs
INCS = -I$(FREETYPEINC)
LIBS = -lX11 -lXext $(XINERAMALIBS) $(FREETYPELIBS)

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"$(VERSION)\" $(XINERAMAFLAGS)
CFLAGS   = -std=c99 -pedantic -Wall -Wno-deprecated-declarations -Os $(INCS) $(CPPFLAGS)
LDFLAGS  = $(LIBS)

# compiler and linker
CC = cc
