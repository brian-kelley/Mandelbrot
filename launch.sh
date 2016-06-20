#!/bin/bash
nice -n 1 build/Mandelbrot --size 200x200 --targetcache target.bin -n 4 --depth -1000 --verbose
