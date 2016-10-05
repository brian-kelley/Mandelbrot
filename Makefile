CFLAGS=-std=c11 -Ofast -march=native -mtune=native -ffast-math -fomit-frame-pointer -fassociative-math
DEBUGFLAGS=-std=c11 -g
FASTCFLAGS=-std=c11 -O0
SIMDFLAGS=-msse -msse2 -msse3 -mavx -mavx2
SOURCES=mandelbrot.c bigint.c fixedpoint.c image.c timing.c lodepng.c

all:
	nasm -f macho64 routines.asm -o routines.o
	gcc ${CFLAGS} ${SIMDFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -lglib-2.0 -o build/Mandelbrot

quick: 
	nasm -f macho64 routines.asm -o routines.o
	gcc ${FASTCFLAGS} ${SIMDFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -lglib-2.0 -o build/Mandelbrot

profiling:
	nasm -f macho64 routines.asm -o routines.o
	gcc ${PROFILINGFLAGS} ${SIMDFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -lglib-2.0 -o build/Mandelbrot

linux:
	nasm -f macho64 routines.asm -o routines.o
	gcc ${CFLAGS} ${SIMDFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -lglib-2.0 -o build/Mandelbrot

debug:
	nasm -f macho64 routines.asm -o routines.o
	gcc ${DEBUGFLAGS} ${SIMDFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -o build/Mandelbrot

bigintAsm:
	gcc ${CFLAGS} -S bigint.c -o bigint.asm

clean:
	rm build/Mandelbrot
