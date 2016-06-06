#!/bin/bash
nice -n 0 build/Mandelbrot --size 1280x800 --targetcache target.bin --usetargetcache -n 3 --verbose --depth -400
