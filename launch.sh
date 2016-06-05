#!/bin/bash
nice -n 10 build/Mandelbrot --size 300x300 --targetcache new.bin -n 4 --verbose --depth -40
