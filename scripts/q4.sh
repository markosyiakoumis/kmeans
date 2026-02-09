#!/bin/bash

make clean && make
perf record -g -- ./build/bin/kmeans ./images/input.jpg ./out/q4output5.jpg 5
perf script | c++filt | ./src/gprof2dot.py -f perf | dot -Tpng -o ./out/q4perf.png