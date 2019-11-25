CC := cc
CCFLAGS := -Os
LDFLAGS := -lm -lX11 -lXft `pkg-config --cflags freetype2`
DEBUGFLAGS := -fsanitize=undefined -fsanitize=address -fno-sanitize-recover -ggdb3
DEPS := cmenu.c cmenu.h config.h
TARGETS := cmenu

DESTDIR := /usr/local/bin

MANPAGE := cmenu.1
MANDIR := /usr/man/man1

all: $(TARGETS)

debug: $(DEPS)
	@echo 'compiling with debug flags...'
	@$(CC) $< $(CCFLAGS) $(LDFLAGS) $(DEBUGFLAGS) -o $(TARGETS)
	@echo 'done.'

clean:
	@echo "installing..."
	@rm -f $(TARGETS) $(DESTDIR)/$(TARGETS)
	@rm -f $(MANDIR)/$(MANPAGE).gz
	@echo "done."

install:
	@echo "installing..."
	@mv $(TARGETS) $(DESTDIR)
	@cp $(MANPAGE) $(MANDIR)
	@gzip -f $(MANDIR)/$(MANPAGE)
	@echo 'done.'

$(TARGETS): $(DEPS)
	@echo 'compiling...'
	@$(CC) $< $(CCFLAGS) $(LDFLAGS) -o $@
	@echo 'done.'
