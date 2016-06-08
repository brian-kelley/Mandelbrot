CFLAGS=-std=c99 -O3 -march=native -mtune=native -ffast-math
DEBUGFLAGS=-std=c99 -g
FASTCFLAGS=-std=c99 -O0
SOURCES=mandelbrot.c bigint.c fixedpoint.c image.c lodepng.c 

all:
	gcc ${CFLAGS} ${SOURCES} -lc -lpthread -o build/Mandelbrot

linux:
	gcc ${CFLAGS} ${SOURCES} -lc -lpthread -o build/Mandelbrot

debug:
	gcc ${DEBUGFLAGS} ${SOURCES} -lc -lpthread -o build/Mandelbrot

fast:
	gcc ${FASTCFLAGS} ${SOURCES} -lc -lpthread -o build/Mandelbrot

bigintAsm:
	gcc ${CFLAGS} -S bigint.c -o bigint.asm

clean:
	rm build/Mandelbrot
