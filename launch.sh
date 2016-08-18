#!/bin/bash
nice -n 1 build/Mandelbrot --size 640x480 --targetcache target.bin --usetargetcache -n 4 --depth -300 --verbose
