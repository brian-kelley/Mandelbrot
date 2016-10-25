#ifdef INTERACTIVE
//Only compile this if SDL2/OpenGL were found by CMake
#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#include "constants.h"

#ifdef __cplusplus
extern "C" void initInteractive(int windowW, int windowH, int imageW, int imageH);
extern "C" void destroyInteractive();
#else
void interactiveMain(int windowW, int windowH, int imageW, int imageH);
#endif

#endif
#endif
