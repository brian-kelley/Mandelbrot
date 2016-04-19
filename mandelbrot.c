#include <SDL.h>
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "lodepng.h"

typedef struct
{
    double real;
    double imag;
} complex;

double mag(complex* num)
{
    return sqrt(num->real * num->real + num->imag * num->imag);
}

double screenX;
double screenY;
double width;
double height;
const int winw = 2560;
const int winh = 1600;
const int maxiter = 500;
SDL_Window* win;

Uint32 getColor(int num)
{
    if(num == maxiter)
        return 0;
    int grn = (num * 23) & 0xFF;
    int blu = (num * 7) & 0xFF;
    return 0xFFFF0000 | blu | grn << 8;
}

int getConvRate(complex c)
{
    const double stop = 2;
    int iter = 0;
    complex z = {0, 0};
    while(mag(&z) < stop)
    {
        complex temp;
        //z = z^2 + c
        temp.real = z.real * z.real - z.imag * z.imag + c.real;
        temp.imag = 2 * z.real * z.imag + c.imag;
        z = temp;
        iter++;
        if(iter == maxiter)
        {
            //assume in the set
            break;
        }
    }
    return iter;
}

void drawBuf(Uint32* pixels)
{
    for(int i = 0; i < winw; i++)
    {
        for(int j = 0; j < winh; j++)
        {
            double realPos = screenX + width * (double) i / winw;
            double imagPos = screenY + height * (double) j / winh;
            complex c = {realPos, imagPos};
            pixels[j * winw + i] = getColor(getConvRate(c));
        }
    }
}

int main()
{
    screenX = -2;
    screenY = -1;
    width = 3;
    height = 2;
    Uint32* pixbuf = (Uint32*) malloc(sizeof(Uint32) * winw * winh);
    drawBuf(pixbuf);
    //ARGB => RGBA
    //fix colors in-place for png writing
    for(int i = 0; i < winw * winh; i++)
    {
        Uint32 temp = pixbuf[i]; 
        pixbuf[i] = 0xFF000000 | (temp & 0xFF << 8) | (temp & 0xFF00 >> 8) | (temp & 0xFF0000 >> 0);
    }
    lodepng_encode32_file("mandel.png", (unsigned char*) pixbuf, winw, winh);
}
