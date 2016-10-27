#ifdef INTERACTIVE
//Only compile this if SDL2/OpenGL were found by CMake
#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#ifdef __cplusplus
extern "C" void interactiveMain(int windowW, int windowH, int imageW, int imageH);
#else
void interactiveMain(int windowW, int windowH, int imageW, int imageH);
#endif

#endif
#endif

