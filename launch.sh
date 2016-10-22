#!/bin/bash
build/Mandelbrot -n 4 --size 2560x1600 --start 10 --verbose --smooth --supersample --depth -300 --color sunset --targetcache newtarget2.bin --usetargetcache
#build/Mandelbrot -n 4 --size 2560x1600 --supersample --smooth --depth -300 --verbose --start 59 --position -0.749999987350877481864108888020013802837969258 0.0010000386882368320131245812300498491327594258
#build/Mandelbrot -n 4 --size 2560x1600 --smooth --supersample --depth -300 --targetcache newtarget1.bin --usetargetcache
#build/Mandelbrot -n 4 --size 2560x1600 --supersample --smooth --depth -300 --position -1.71977 0
#build/Mandelbrot -n 4 --size 100x100 --supersample --depth -300 --targetcache target2.bin --usetargetcache

