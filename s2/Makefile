include config.mk

SRC = main.c util.c image_io.c editor.c clipboard.c draw.c
OBJ = ${SRC:.c=.o}

all: options s2

options:
	@echo s2 build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "CPPFLAGS = ${CPPFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "LDLIBS   = ${LDLIBS}"

config.h:
	cp config.def.h $@

${OBJ}: config.h config.mk

s2: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS} ${LDLIBS}

.c.o:
	${CC} -c ${CFLAGS} ${CPPFLAGS} $<

clean:
	rm -f s2 ${OBJ}

distclean: clean
	rm -f config.h

dist: clean
	mkdir -p s2-${VERSION}
	cp -R LICENSE Makefile README.md config.mk config.def.h *.c *.h SPEC.md s2-${VERSION}
	tar -cf s2-${VERSION}.tar s2-${VERSION}
	gzip s2-${VERSION}.tar
	rm -rf s2-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	install -m 755 s2 ${DESTDIR}${PREFIX}/bin

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/s2

.PHONY: all options clean distclean dist install uninstall
