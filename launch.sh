#!/bin/bash
nice -n 1 build/Mandelbrot --size 2560x1600 --targetcache target.bin -n 4 --depth -9 --verbose
