PKGS      = glib-2.0 ncursesw

INCS     := $(shell pkg-config --cflags $(PKGS)) -I./
CFLAGS   := -std=gnu23 -ggdb -W -Wall -Wextra $(INCS) $(CFLAGS)

LIBS     := $(shell pkg-config --libs $(PKGS)) -lreadline
LDFLAGS  := $(LIBS) $(LDFLAGS) -Wl,--export-dynamic

SRCS  = $(wildcard *.c)
HEADS = $(wildcard *.h)
OBJS  = $(foreach obj,$(SRCS:.c=.o),$(obj))

PREFIX       ?= /usr
MANPREFIX    ?= $(PREFIX)/share/man
CONFIGPREFIX ?= $(PREFIX)/etc/xdg/chooser

INSTALLDIR := $(DESTDIR)$(PREFIX)
MANDIR     := $(DESTDIR)$(MANPREFIX)
CONFIGDIR  := $(DESTDIR)$(CONFIGPREFIX)

chooser: $(OBJS)
	@echo $(CC) -o $@ $(OBJS)
	@$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): $(HEADS)

.c.o:
	@echo $(CC) -c $< -o $@
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	rm -f chooser $(OBJS)

all: chooser

install:
	install -d $(INSTALLDIR)/bin
	install -m 755 chooser $(INSTALLDIR)/bin/chooser
	install -d $(CONFIGDIR)
	install -m 644 config/* $(CONFIGDIR)

uninstall:
	rm -f $(INSTALLDIR)/bin/chooser
	rm -rf $(CONFIGDIR)
