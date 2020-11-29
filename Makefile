COMMON_CFLAGS=-Wall -Wextra -std=c99 -pedantic
CFLAGS=`pkg-config --cflags sdl2` $(COMMON_CFLAGS)
COMMON_LIBS=-lm
LIBS=`pkg-config --libs sdl2` $(COMMON_LIBS)

sowon: main.c digits.h
	$(CC) $(CFLAGS) -o sowon main.c $(LIBS)

digits.h: png2c digits.png
	./png2c digits.png > digits.h

png2c: png2c.c
	$(CC) $(COMMON_CFLAGS) -o png2c png2c.c -lm
