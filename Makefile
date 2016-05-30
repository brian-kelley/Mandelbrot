CFLAGS=-std=c99 -O3 -march=native -mtune=native -ffast-math
DEBUGFLAGS=-std=c99 -g
FASTCFLAGS=-std=c99 -O0
SOURCES=mandelbrot.c bigint.c fixedpoint.c lodepng.c

all:
	nasm -f macho64 routines.asm -o build/routines.o 
	gcc ${CFLAGS} ${SOURCES} build/routines.o -lc -lpthread -o build/Mandelbrot

linux:
	nasm -f elf64 routinesLinux.asm -o build/routines.o 
	gcc ${CFLAGS} ${SOURCES} build/routines.o -lc -lpthread -o build/Mandelbrot

debug:
	nasm -f macho64 routines.asm -o build/routines.o 
	gcc ${DEBUGFLAGS} ${SOURCES} build/routines.o -lc -lpthread -o build/Mandelbrot

fast:
	nasm -f macho64 routines.asm -o build/routines.o 
	gcc ${FASTCFLAGS} ${SOURCES} build/routines.o -lc -lpthread -o build/Mandelbrot

clean:
	rm build/Mandelbrot
