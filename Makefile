CFLAGS=-std=c99 -O3 -ffast-math
FASTCFLAGS=-std=c99

all:
	nasm -f macho64 routines.asm -o build/routines.o 
	gcc ${CFLAGS} mandelbrot.c lodepng.c precision.c build/routines.o -lc -lpthread -o build/Mandelbrot

fast:
	nasm -f macho64 routines.asm -o build/routines.o 
	gcc ${FASTCFLAGS} mandelbrot.c lodepng.c precision.c build/routines.o -lc -lpthread -o build/Mandelbrot

clean:
	rm build/*.o
	rm build/Mandelbrot
