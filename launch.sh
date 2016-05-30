#!/bin/bash
nice -n 10 build/Mandelbrot --size 200x200 --usetargetcache --targetcache new.bin -n 1
