#!/bin/bash
lldb build/Mandelbrot -- build/Mandelbrot -n 1 --size 1280x720 --usetargetcache --targetcache target2.bin --depth -300 --verbose --seed 42
