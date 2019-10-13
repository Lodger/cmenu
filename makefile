CC := cc
CCFLAGS := -lX11 -lXft -Os
LDFLAGS := `pkg-config --cflags freetype2`
DEBUGFLAGS := -fsanitize=undefined -fsanitize=address -fno-sanitize-recover -ggdb3
DEPS := cmenu.c cmenu.h config.h
TARGETS := cmenu

DESTDIR := /usr/bin

MANPAGE := cmenu.1
MANDIR := /usr/man/man1

all: $(TARGETS)

debug: $(DEPS)
	@echo 'compiling with debug flags...'
	@$(CC) $< $(CCFLAGS) $(LDFLAGS) $(DEBUGFLAGS) -o $(TARGETS)
	@echo 'done.'

clean:
	rm -f $(TARGETS) $(DESTDIR)/$(TARGETS)
	rm -f $(MANDIR)/$(MANPAGE).gz

install:
	mv $(TARGETS) $(DESTDIR)
	cp $(MANPAGE) $(MANDIR)
	gzip $(MANDIR)/$(MANPAGE)

$(TARGETS): $(DEPS)
	@echo 'compiling...'
	@$(CC) $< $(CCFLAGS) $(LDFLAGS) -o $@
	@echo 'done.'
