CFLAGS=-std=c99 -O3 -march=native -mtune=native -ffast-math
DEBUGFLAGS=-std=c99 -O3 -march=native -mtune=native -ffast-math -g
FASTCFLAGS=-std=c99 -Oz

all:
	nasm -f macho64 routines.asm -o build/routines.o 
	gcc ${CFLAGS} mandelbrot.c lodepng.c precision.c build/routines.o -lc -lpthread -o build/Mandelbrot

linux:
	nasm -f elf64 routines.asm -o build/routines.o 
	gcc ${CFLAGS} mandelbrot.c lodepng.c precision.c build/routines.o -lc -lpthread -o build/Mandelbrot

debug:
	nasm -f macho64 routines.asm -o build/routines.o 
	gcc ${DEBUGFLAGS} mandelbrot.c lodepng.c precision.c build/routines.o -lc -lpthread -o build/Mandelbrot

fast:
	nasm -f macho64 routines.asm -o build/routines.o 
	gcc ${FASTCFLAGS} mandelbrot.c lodepng.c precision.c build/routines.o -lc -lpthread -o build/Mandelbrot

clean:
	rm build/*.o
	rm build/Mandelbrot
