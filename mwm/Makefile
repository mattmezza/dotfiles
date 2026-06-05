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

mwmc.o: mwmc.c ipc.h util.h config.mk completions.h

# embed the shell completion scripts into mwmc (so the binary alone can emit
# them via `mwmc completions bash|zsh`). Each file is turned into a C string
# literal: escape backslashes, then double-quotes, then wrap each line.
completions.h: completions/mwmc.bash completions/mwmc.zsh
	{ \
	printf 'static const char completion_bash[] =\n'; \
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$$/\\n"/' completions/mwmc.bash; \
	printf ';\nstatic const char completion_zsh[] =\n'; \
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$$/\\n"/' completions/mwmc.zsh; \
	printf ';\n'; \
	} > $@

clean:
	rm -f mwm mwmc $(OBJ) mwmc.o config.h completions.h mwm-$(VERSION).tar.gz

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f mwm mwmc $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/mwm
	chmod 755 $(DESTDIR)$(PREFIX)/bin/mwmc
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" mwm.1 > $(DESTDIR)$(MANPREFIX)/man1/mwm.1 2>/dev/null || true
	sed "s/VERSION/$(VERSION)/g" mwmc.1 > $(DESTDIR)$(MANPREFIX)/man1/mwmc.1 2>/dev/null || true
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/mwm.1 2>/dev/null || true
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/mwmc.1 2>/dev/null || true
	mkdir -p $(DESTDIR)$(PREFIX)/share/bash-completion/completions
	cp -f completions/mwmc.bash $(DESTDIR)$(PREFIX)/share/bash-completion/completions/mwmc
	mkdir -p $(DESTDIR)$(PREFIX)/share/zsh/site-functions
	cp -f completions/mwmc.zsh $(DESTDIR)$(PREFIX)/share/zsh/site-functions/_mwmc

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/mwm \
	      $(DESTDIR)$(PREFIX)/bin/mwmc \
	      $(DESTDIR)$(MANPREFIX)/man1/mwm.1 \
	      $(DESTDIR)$(MANPREFIX)/man1/mwmc.1 \
	      $(DESTDIR)$(PREFIX)/share/bash-completion/completions/mwmc \
	      $(DESTDIR)$(PREFIX)/share/zsh/site-functions/_mwmc

# --- vagrant test VM helpers (run on host) ---
vmup:
	vagrant up --provider=libvirt

vmconnect: vmup
	virt-viewer --connect qemu:///system --attach mwm_default

vmdown:
	vagrant destroy

.PHONY: all clean install uninstall vmup vmconnect
