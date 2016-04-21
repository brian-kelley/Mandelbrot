#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include "pthread.h"
#include "lodepng.h"

#define max(a, b) (a < b ? b : a);
#define min(a, b) (a < b ? a : b);

typedef uint32_t Uint32;
typedef long double real;

typedef struct
{
    real r;
    real i;
} complex;
real magSquared(complex* z) { return z->r * z->r + z->i * z->i; }

#define winw 640
#define winh 400
#define zoomFactor 1.5
#define numImages 150
#define totalIter (1 << 17)
real screenX;
real screenY;
real width;
real height;
Uint32* pixbuf;
int* iterbuf;
Uint32* colortable;
int filecount;
int numThreads;
int maxiter;

void writeImage()
{
    char name[32];
    sprintf(name, "output/mandel%i.png", filecount++);
    for(int i = 0; i < winw * winh; i++)
    {
        Uint32 temp = pixbuf[i]; 
        pixbuf[i] = 0xFF000000 | (temp & 0xFF << 8) | (temp & 0xFF00 >> 8) | (temp & 0xFF0000 >> 0);
    }
    lodepng_encode32_file(name, (unsigned char*) pixbuf, winw, winh);
}

void zoom()
{
    //get the highest (non-convergent) iteration count
    //in the middle 1/9 of window area
    int bestX = 0;
    int bestY = 0;
    int bestIter = 0;
    for(int i = winw / 3; i < winw * 2 / 3; i++)
    {
        for(int j = winh / 3; j < winh * 2 / 3; j++)
        {
            if(iterbuf[i + j * winw] > bestIter)
            {
                bestX = i;
                bestY = j;
                bestIter = iterbuf[i + j * winw];
            }
        }
    }
    printf("Deepest pixel has %i iters.\n", bestIter);
    //now zoom to (bestX, bestY)
    //zoom in by factor of 1.5
    //get the decrease in width and height
    real dw = width * (1 - 1 / zoomFactor);
    real dh = height * (1 - 1 / zoomFactor);
    screenX += dw * ((real) bestX / winw);
    screenY += dh * ((real) bestY / winh);
    width -= dw;
    height -= dh;
}

void initColorTable()
{
    for(int i = 0; i < totalIter; i++)
    {
        int red = abs(((i) % 512 + 256) - 256);
        int grn = abs((((i + 150) * 2) % 512) - 256);
        int blu = abs(((i - grn) % 512) - 256);
        colortable[i] = 0xFF000000 | red << 0 | grn << 8 | blu << 16;
    }
}

Uint32 getColor(int num)
{
    if(num == -1)
        return 0xFF000000;                  //in the mandelbrot set = opaque black
    return colortable[num % 512];
}

int getConvRate(complex c)
{
    const real stop = 4;
    int iter = 0;
    complex z = {0, 0};
    while(magSquared(&z) < stop)
    {
        complex temp;
        //z = z^2 + c
        temp.r = z.r * z.r - z.i * z.i + c.r;
        temp.i = 2 * z.r * z.i + c.i;
        z = temp;
        iter++;
        if(iter == maxiter)
        {
            //not diverging
            break;
        }
    }
    return iter == maxiter ? -1 : iter;
}

void* workerFunc(void* indexAsInt)
{
    int index = *((int*) indexAsInt);
    int colsPerThread = winw / numThreads;
    for(int i = colsPerThread * index; i < colsPerThread * (index + 1); i++)
    {
        for(int j = 0; j < winh; j++)
        {
            real realPos = screenX + width * (real) i / winw;
            real imagPos = screenY + height * (real) j / winh;
            complex c = {realPos, imagPos};
            int convRate = getConvRate(c);
            pixbuf[j * winw + i] = getColor(convRate);
            iterbuf[j * winw + i] = convRate;
        }
    }
    return NULL;
}

void drawBuf()
{
    if(numThreads == 1)
    {
        for(int i = 0; i < winw; i++)
        {
            for(int j = 0; j < winh; j++)
            {
                real realPos = screenX + width * (real) i / winw;
                real imagPos = screenY + height * (real) j / winh;
                complex c = {realPos, imagPos};
                int convRate = getConvRate(c);
                pixbuf[j * winw + i] = getColor(convRate);
                iterbuf[j * winw + i] = convRate;
            }
        }
        return;
    }
    pthread_t* threads = (pthread_t*) malloc(sizeof(pthread_t) * numThreads);
    int* indices = (int*) malloc(sizeof(int) * numThreads);
    for(int i = 0; i < numThreads; i++)
    {
        indices[i] = i;
        if(pthread_create(&threads[i], NULL, workerFunc, indices + i))
        {
            puts("Failed to create thread.");
            exit(EXIT_FAILURE);
        }
    }
    for(int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }
    free(indices);
    free(threads);
}

int main(int argc, const char** argv)
{
    maxiter = 500;
    colortable = (Uint32*) malloc(sizeof(Uint32) * maxiter);
    initColorTable();
    screenX = -2;
    screenY = -1;
    width = 3;
    height = 2;
    if(argc == 2)
        sscanf(argv[1], "%i", &numThreads);
    else
        numThreads = 4;
    printf("Running on %i threads.\n", numThreads);
    pixbuf = (Uint32*) malloc(sizeof(Uint32) * winw * winh);
    iterbuf = (int*) malloc(sizeof(int) * winw * winh);
    filecount = 0;
    for(int i = 0; i < 150; i++)
    {
        clock_t start = clock();
        drawBuf();
        writeImage();
        zoom();
        maxiter *= 1.05;
        if(maxiter > totalIter)
            maxiter = totalIter;
        real time = (real) (clock() - start) / CLOCKS_PER_SEC;
        printf("Generated image #%i in %Lf seconds.\n", filecount - 1, time);
    }
    free(iterbuf);
    free(pixbuf);
    free(colortable);
}
