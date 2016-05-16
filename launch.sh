#!/bin/bash
nice -n 10 build/Mandelbrot --size 2560x1600 --usetargetcache --targetcache stem.bin -n 6
