cmake_minimum_required (VERSION 3.1)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk)
project (Mandelbrot)
set(CMAKE_C_FLAGS "-std=c11 -Wall -O3 -march=native -mtune=native -mavx2 -ffast-math -fomit-frame-pointer -fassociative-math -funroll-loops")
set(CMAKE_CXX_FLAGS "-std=c++11 -O2 -Wno-extern-c-compat")

find_library(PTHREAD_LIB pthread)
if(NOT PTHREAD_LIB)
  message(FATAL_ERROR "pthread library not found.")
endif()

option(INTERACTIVE "Enable interactive GUI" ON)

#Do common executable setup

include_directories(/usr/local/include)

# Optional interactive support
# Requires: SDL2, OpenGL 2+
if(INTERACTIVE)
  find_library(SDL2_LIB SDL2)
  if(NOT SDL2_LIB)
    message(FATAL_ERROR "SDL2 library not found.")
  endif()
  find_library(GL_LIB OpenGL)
  if(NOT GL_LIB)
    message(FATAL_ERROR "OpenGL library not found.")
  endif()
  include_directories(imgui)
  add_executable(Mandelbrot
    mandelbrot.c
    bigint.c
    fixedpoint.c
    image.c
    boundary.c
    lodepng.c
    interactive.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_impl_sdl.cpp
  )
  message("** GUI support enabled")
  add_definitions(-DINTERACTIVE)
  target_link_libraries(Mandelbrot ${SDL2_LIB} ${GL_LIB} ${PTHREAD_LIB})
else()
  message("** Command line interface only")
  message("** Install SDL2 for GUI support")
  add_executable(Mandelbrot
    mandelbrot.c
    bigint.c
    fixedpoint.c
    image.c
    boundary.c
    lodepng.c
  )
  # TODO: make this portable
  target_link_libraries(${PTHREAD_LIB})
endif()

