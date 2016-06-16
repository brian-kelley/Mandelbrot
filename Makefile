CFLAGS=-std=c99 -Ofast -march=native -mtune=native -ffast-math
DEBUGFLAGS=-std=c99 -g
FASTCFLAGS=-std=c99 -O0
SOURCES=mandelbrot.c bigint.c fixedpoint.c image.c lodepng.c 

all:
	nasm -f macho64 routines.asm -o routines.o
	gcc ${CFLAGS} ${SOURCES} routines.o -lc -lpthread -o build/Mandelbrot

linux:
	nasm -f macho64 routines.asm -o routines.o
	gcc ${CFLAGS} ${SOURCES} routines.o -lc -lpthread -o build/Mandelbrot

debug:
	nasm -f macho64 routines.asm -o routines.o
	gcc ${DEBUGFLAGS} ${SOURCES} routines.o -lc -lpthread -o build/Mandelbrot

fast:
	nasm -f macho64 routines.asm -o routines.o
	gcc ${FASTCFLAGS} ${SOURCES} routines.o -lc -lpthread -o build/Mandelbrot

bigintAsm:
	gcc ${CFLAGS} -S bigint.c -o bigint.asm

clean:
	rm build/Mandelbrot
