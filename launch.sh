#!/bin/bash
nice -n 10 build/Mandelbrot --size 500x500 --targetcache test.bin -n 4
