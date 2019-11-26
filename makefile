CC := gcc
CCFLAGS := -Os
LDFLAGS := -lm -lX11 -lXft `pkg-config --cflags freetype2`
DEBUGFLAGS := -Wall -fsanitize=undefined -fsanitize=address -fno-sanitize-recover -ggdb3

DESTDIR := /usr/local/bin
TARGET := cmenu
DEPS := util.o

MANDIR := /usr/local/man/man1
MANPAGE := cmenu.1

all: $(TARGET)

debug: CCFLAGS += $(DEBUGFLAGS)
debug: $(TARGET)

clean:
	@echo "cleaning..."
	rm -f $(DEPS)
	rm -f $(DESTDIR)/$(TARGET)
	rm -f $(MANDIR)/$(MANPAGE)
	@echo "done."

install:
	@echo "installing..."
	mv -f $(TARGET) $(DESTDIR)
	install -m644 $(MANPAGE) $(MANDIR)/$(MANPAGE)
	@echo "done."

%.o: %.c 
	@echo "building object files..."
	$(CC) -c $(CCFLAGS) $(LDFLAGS) $<
	@echo "done."

$(TARGET): util.o
	@echo "compiling..."
	$(CC) $(CCFLAGS) $(LDFLAGS) cmenu.c util.o -o cmenu
	@echo "done."
