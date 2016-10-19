#!/bin/bash
lldb build/Mandelbrot -- -n 4 --size 2560x1600 --smooth --supersample --depth -300 --verbose --start 0 --color sunset --targetcache target.bin --usetargetcache
