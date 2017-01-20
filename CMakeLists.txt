cmake_minimum_required (VERSION 3.1)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk)
project (Mandelbrot)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -std=c11 -Wall -march=native -mavx2 -ffast-math -fassociative-math")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -Wall -Wno-extern-c-compat")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -std=c11 -Wall -O3 -march=native -mtune=native -mavx2 -ffast-math -fomit-frame-pointer -fassociative-math -funroll-loops")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11 -O2 -Wno-extern-c-compat")

find_library(PTHREAD_LIB pthread)
if(NOT PTHREAD_LIB)
  message(FATAL_ERROR "pthread library not found.")
endif()

#Do common executable setup

include_directories(/usr/local/include)

# Interactive requires: SDL2, OpenGL 2+
find_library(SDL2_LIB SDL2)
find_library(GL_LIB OpenGL)
find_library(CL_LIB OpenCL)

# Create Mandelbrot executable
include_directories(imgui)
add_executable(Mandelbrot
  main.c
  mandelbrot.c
  bigint.c
  fixedpoint.c
  specialMath.c
  image.c
  bitset.c
  test.c
  lodepng.c
  interactive.cpp
  #gpu.cpp
)

target_link_libraries(Mandelbrot ${PTHREAD_LIB})

if(SDL2_LIB AND GL_LIB)
  message("GUI support enabled")
  add_definitions(-DINTERACTIVE)
  target_link_libraries(Mandelbrot ${SDL2_LIB} ${GL_LIB})
else()
  message("Install OpenGL and SDL2 for GUI support")
endif()

if(CL_LIB)
  message("OpenCL support enabled")
  add_definitions(-DGPU)
  target_link_libraries(Mandelbrot ${CL_LIB})
else()
  message("Install OpenCL for GPU support")
endif()

