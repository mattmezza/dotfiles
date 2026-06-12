# lok - a pretty screen locker
# See LICENSE file for copyright and license details.

include config.mk

SRC = lok.c
OBJ = $(SRC:.c=.o)

all: lok

.c.o:
	$(CC) -c $(CFLAGS) $<

$(OBJ): arg.h config.h config.mk

config.h:
	cp config.def.h $@

lok: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f config.h
	rm -f lok $(OBJ) lok-$(VERSION).tar.gz test/shim.so

dist: clean
	mkdir -p lok-$(VERSION)
	cp -R LICENSE Makefile README.md config.mk config.def.h arg.h \
		lok.1 $(SRC) lok-$(VERSION)
	tar -cf - lok-$(VERSION) | gzip > lok-$(VERSION).tar.gz
	rm -rf lok-$(VERSION)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f lok $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/lok
	chmod u+s $(DESTDIR)$(PREFIX)/bin/lok
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < lok.1 > $(DESTDIR)$(MANPREFIX)/man1/lok.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/lok.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/lok
	rm -f $(DESTDIR)$(MANPREFIX)/man1/lok.1

test: all
	sh test/run.sh

.PHONY: all clean dist install uninstall test
