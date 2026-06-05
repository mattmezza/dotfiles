# mwm - matteo's window manager
.POSIX:

include config.mk

SRC = drw.c mwm.c util.c
OBJ = $(SRC:.c=.o)

all: mwm mwmc

.c.o:
	$(CC) -c $(CFLAGS) $<

$(OBJ): config.h config.mk drw.h util.h ipc.h

config.h:
	cp config.def.h $@

mwm: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

mwmc: mwmc.o util.o
	$(CC) -o $@ mwmc.o util.o $(LDFLAGS)

mwmc.o: mwmc.c ipc.h util.h config.mk

clean:
	rm -f mwm mwmc $(OBJ) mwmc.o config.h mwm-$(VERSION).tar.gz

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f mwm mwmc $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/mwm
	chmod 755 $(DESTDIR)$(PREFIX)/bin/mwmc
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" mwm.1 > $(DESTDIR)$(MANPREFIX)/man1/mwm.1 2>/dev/null || true
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/mwm.1 2>/dev/null || true

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/mwm \
	      $(DESTDIR)$(PREFIX)/bin/mwmc \
	      $(DESTDIR)$(MANPREFIX)/man1/mwm.1

# --- vagrant test VM helpers (run on host) ---
vmup:
	vagrant up --provider=libvirt

vmconnect: vmup
	virt-viewer --connect qemu:///system --attach mwm_default

vmdown:
	vagrant destroy

.PHONY: all clean install uninstall vmup vmconnect
