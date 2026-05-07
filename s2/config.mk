VERSION = 0.1

PREFIX = /usr/local
X11INC = /usr/include/X11
X11LIB = /usr/lib

INCS = -I. -I${X11INC}
INCS += $(shell pkg-config --cflags xft)
INCS += $(shell pkg-config --cflags pangocairo)
LIBS = -L${X11LIB} -lX11 -lpng -lm $(shell pkg-config --libs xft)
LIBS += $(shell pkg-config --libs pangocairo)

CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -DVERSION=\"${VERSION}\"
CFLAGS = -std=c99 -pedantic -Wall -Wextra -O2 ${INCS}
LDFLAGS = ${LIBS}
LDLIBS =

CC = cc
