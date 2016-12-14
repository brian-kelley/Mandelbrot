//Only compile this if SDL2/OpenGL were found by CMake
#ifdef INTERACTIVE
#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#include "constants.h"

MANDELBROT_API void interactiveMain(int imageW, int imageH);

#endif
#endif

