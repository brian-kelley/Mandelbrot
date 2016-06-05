#!/bin/bash

lldb build/Mandelbrot -- --nice -n 10 build/Mandelbrot --size 300x300 --targetcache target.bin -n 4 --verbose
