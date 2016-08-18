#!/bin/bash
lldb build/Mandelbrot -- build/Mandelbrot --size 800x800 --targetcache target.bin -n 1 --depth -30 --verbose
