#!/bin/bash
lldb build/Mandelbrot -- build/Mandelbrot -n 4 --size 1280x720 --usetargetcache --targetcache target.bin --depth -300 --verbose
