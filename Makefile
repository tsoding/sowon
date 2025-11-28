UNAMEOS = $(shell uname)

COMMON_CFLAGS=		-Wall -Wextra -ggdb -std=c99 -pedantic -Ithirdparty -Ibuild -DPENGER
COMMON_LIBS=		-lm
SDL2_LIBS=			`pkg-config --libs sdl2` $(COMMON_LIBS)
ifeq ($(UNAMEOS),Darwin)
RGFW_LIBS=			$(COMMON_LIBS) -framework CoreVideo -framework Cocoa -framework OpenGL -framework IOKit
else
RGFW_LIBS=			-lX11 -lXrandr -lGLX -lGL $(COMMON_LIBS)
endif
PREFIX?=		/usr/local
INSTALL?=		install

.PHONY: all
all: Makefile sowon sowon_rgfw man

sowon_rgfw: src/main_rgfw.c build/digits.h build/penger_walk_sheet.h
	$(CC) $(COMMON_CFLAGS) -o sowon_rgfw src/main_rgfw.c $(RGFW_LIBS)

sowon: src/main.c build/digits.h build/penger_walk_sheet.h
	$(CC) $(COMMON_CFLAGS) -o sowon src/main.c $(SDL2_LIBS)

build/digits.h: build/png2c ./assets/digits.png
	./build/png2c ./assets/digits.png digits > build/digits.h

build/penger_walk_sheet.h: build/png2c ./assets/penger_walk_sheet.png
	./build/png2c ./assets/penger_walk_sheet.png penger > build/penger_walk_sheet.h

build/png2c: src/png2c.c | build
	$(CC) $(COMMON_CFLAGS) -o build/png2c src/png2c.c -lm

docs/sowon.6.gz: docs/sowon.6
	gzip -c docs/sowon.6 > docs/sowon.6.gz

build:
	mkdir -pv build

.PHONY: man
man: docs/sowon.6.gz

.PHONY: clean
clean:
	rm -r sowon sowon_rgfw build docs/sowon.6.gz

.PHONY: install
install: all
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -C ./sowon $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -C ./sowon_rgfw $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/man/man6
	$(INSTALL) -C docs/sowon.6.gz $(DESTDIR)$(PREFIX)/man/man6
