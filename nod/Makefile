# nod - suckless notification daemon

include config.mk

SRC = nod.c
OBJ = $(SRC:.c=.o)

all: nod

config.h:
	cp config.def.h $@

.c.o:
	$(CC) -c $(CFLAGS) $<

$(OBJ): config.h config.mk

nod: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f nod $(OBJ)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f nod $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/nod
	cp -f nod-send $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/nod-send
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < nod.1      > $(DESTDIR)$(MANPREFIX)/man1/nod.1
	sed "s/VERSION/$(VERSION)/g" < nod-send.1 > $(DESTDIR)$(MANPREFIX)/man1/nod-send.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/nod.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/nod-send.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/nod
	rm -f $(DESTDIR)$(PREFIX)/bin/nod-send
	rm -f $(DESTDIR)$(MANPREFIX)/man1/nod.1
	rm -f $(DESTDIR)$(MANPREFIX)/man1/nod-send.1

.PHONY: all clean install uninstall
