# lok - a pretty screen locker
# See LICENSE file for copyright and license details.

include config.mk

SRC = lok.c fingerprint.c
OBJ = $(SRC:.c=.o)

all: lok

.c.o:
	$(CC) -c $(CFLAGS) $<

$(OBJ): arg.h config.h config.mk fingerprint.h

config.h:
	cp config.def.h $@

lok: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f config.h
	rm -f lok $(OBJ) lok-$(VERSION).tar.gz test/shim.so test/fingerprint

dist: clean
	mkdir -p lok-$(VERSION)
	cp -R LICENSE Makefile README.md config.mk config.def.h arg.h \
		lok.1 fingerprint.h pam.d test $(SRC) lok-$(VERSION)
	tar -cf - lok-$(VERSION) | gzip > lok-$(VERSION).tar.gz
	rm -rf lok-$(VERSION)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -o root -g root -m 4755 lok $(DESTDIR)$(PREFIX)/bin/lok
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < lok.1 > $(DESTDIR)$(MANPREFIX)/man1/lok.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/lok.1

install-pam:
	install -d $(DESTDIR)/etc/pam.d
	install -o root -g root -m 644 pam.d/lok-fingerprint $(DESTDIR)/etc/pam.d/lok-fingerprint

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/lok
	rm -f $(DESTDIR)$(MANPREFIX)/man1/lok.1

test: all
	$(CC) $(CFLAGS) -o test/fingerprint test/fingerprint.c
	./test/fingerprint
	sh test/run.sh

.PHONY: all clean dist install install-pam uninstall test
