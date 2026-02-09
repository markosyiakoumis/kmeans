#!/bin/bash

make clean && make PG=1
./build/bin/kmeans ./images/input.jpg ./out/q3output5.jpg 5
gprof ./build/bin/kmeans gmon.out | ./src/gprof2dot.py | dot -Tpng -o ./out/q3gprof.png
