CFLAGS=-std=c11 -Ofast -march=native -mtune=native -masm=intel -ffast-math -fomit-frame-pointer -fassociative-math -funroll-loops
SMALLFLAGS=-std=c11 -Os -fomit-frame-pointer -fassociative-math -ffast-math
DEBUGFLAGS=-std=c11 -g
FASTCFLAGS=-std=c11 -O0
SIMDFLAGS=-msse -msse2 -msse3 -mavx -mavx2
SOURCES=mandelbrot.c bigint.c fixedpoint.c image.c kernels.c boundary.c lodepng.c

all: BuildDir OutputDir
	nasm -f macho64 routines.asm -o routines.o
	gcc ${CFLAGS} ${SIMDFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -lglib-2.0 -o build/Mandelbrot

small: BuildDir OutputDir
	nasm -f macho64 routines.asm -o routines.o
	gcc ${SMALLFLAGS} ${SIMDFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -lglib-2.0 -o build/Mandelbrot

kernelAsm:
	gcc ${CFLAGS} ${SIMDFLAGS} ${INCLUDE} kernels.c -S -o build/Mandelbrot.asm

quick: BuildDir OutputDir
	nasm -f macho64 routines.asm -o routines.o
	gcc ${FASTCFLAGS} ${SIMDFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -lglib-2.0 -o build/Mandelbrot

profiling: BuildDir OutputDir
	nasm -f macho64 routines.asm -o routines.o
	gcc ${PROFILINGFLAGS} ${SIMDFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -lglib-2.0 -o build/Mandelbrot

linux: BuildDir OutputDir
	nasm -f macho64 routines.asm -o routines.o
	gcc ${CFLAGS} ${SIMDFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -lglib-2.0 -o build/Mandelbrot

debug: BuildDir OutputDir
	nasm -f macho64 routines.asm -o routines.o
	gcc ${DEBUGFLAGS} ${SIMDFLAGS} ${INCLUDE} ${SOURCES} routines.o -lc -lpthread -o build/Mandelbrot

bigintAsm:
	gcc ${CFLAGS} -S bigint.c -o bigint.asm

BuildDir:
	if ! [ -a build ] ; \
	then \
		mkdir build ; \
	fi;

OutputDir:
	if ! [ -a output ] ; \
	then \
		mkdir output ; \
	fi;

clean:
	rm -rf build/*
