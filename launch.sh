#!/bin/bash
nice -n 0 build/Mandelbrot --size 640x480 --targetcache target.bin --usetargetcache -n 3 --verbose --depth -400
