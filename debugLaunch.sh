#!/bin/bash

lldb build/Mandelbrot -- build/Mandelbrot --size 1920x1080 --targetcache target.bin --usetargetcache -n 3 --verbose
