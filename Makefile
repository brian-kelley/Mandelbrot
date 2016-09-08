CFLAGS=-std=c11 -Ofast -march=native -mtune=native -ffast-math
DEBUGFLAGS=-std=c11 -g
FASTCFLAGS=-std=c11 -O0
SOURCES=mandelbrot.c bigint.c fixedpoint.c image.c lodepng.c
INCLUDE=#-I/usr/local/include/glib-2.0 -I/usr/local/include/glib-2.0/glib

all:
	nasm -f macho64 routines.asm -o routines.o
	gcc ${CFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -lglib-2.0 -o build/Mandelbrot

quick: 
	nasm -f macho64 routines.asm -o routines.o
	gcc ${FASTCFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -lglib-2.0 -o build/Mandelbrot

linux:
	nasm -f macho64 routines.asm -o routines.o
	gcc ${CFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -lglib-2.0 -o build/Mandelbrot

debug:
	nasm -f macho64 routines.asm -o routines.o
	gcc ${DEBUGFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -o build/Mandelbrot

bigintAsm:
	gcc ${CFLAGS} -S bigint.c -o bigint.asm

clean:
	rm build/Mandelbrot
