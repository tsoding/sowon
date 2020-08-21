CFLAGS=`pkg-config --cflags sdl2` -Wall -Wextra -std=c99 -pedantic
LIBS=`pkg-config --libs sdl2` -lm

sowon: main.c
	$(CC) $(CFLAGS) -o sowon main.c $(LIBS)
