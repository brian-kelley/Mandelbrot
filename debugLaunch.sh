#!/bin/bash
lldb build/Mandelbrot -- -n 4 --size 320x200 --smooth --depth -300 --targetcache target2.bin --usetargetcache
