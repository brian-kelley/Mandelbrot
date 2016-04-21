#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include "pthread.h"
#include "lodepng.h"

typedef struct
{
    double real;
    double imag;
} complex;

double magSquared(complex* z)
{
    return z->real * z->real + z->imag * z->imag;
}

typedef uint32_t Uint32;

#define winw 320
#define winh 200
#define maxiter 40000
double screenX;
double screenY;
double width;
double height;
Uint32* pixbuf;
int* iterbuf;
int filecount;
int numThreads;

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
    const double zoomFactor = 1.5;
    //get the decrease in width and height
    double dw = width * (1 - 1 / zoomFactor);
    double dh = height * (1 - 1 / zoomFactor);
    screenX += dw * ((double) bestX / winw);
    screenY += dh * ((double) bestY / winh);
    width -= dw;
    height -= dh;
}

Uint32 getColor(int num)
{
    if(num == -1)
        return 0xFF000000;                  //in the mandelbrot set = opaque black
    num = sqrt(num);
    unsigned char red = 0xFF;
    unsigned char grn = (num * 29) % 255;
    unsigned char blu = (num * 7) % 255;
    return 0xFF000000 | blu | grn << 8 | red << 16;
}

int getConvRate(complex c)
{
    const double stop = 4;
    int iter = 0;
    complex z = {0, 0};
    while(magSquared(&z) < stop)
    {
        complex temp;
        //z = z^2 + c
        temp.real = z.real * z.real - z.imag * z.imag + c.real;
        temp.imag = 2 * z.real * z.imag + c.imag;
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
            double realPos = screenX + width * (double) i / winw;
            double imagPos = screenY + height * (double) j / winh;
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
                double realPos = screenX + width * (double) i / winw;
                double imagPos = screenY + height * (double) j / winh;
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
    for(int i = 0; i < 40; i++)
    {
        clock_t start = clock();
        drawBuf();
        writeImage();
        zoom();
        double time = (double) (clock() - start) / CLOCKS_PER_SEC;
        printf("Generated image #%i in %f seconds.\n", filecount - 1, time);
    }
    free(iterbuf);
    free(pixbuf);
}
