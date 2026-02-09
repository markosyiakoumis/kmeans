#!/bin/bash

make clean && make OPTIMIZE=O0
perf stat -e instructions,cycles,cache-references,cache-misses,branch-instructions,branch-misses ./build/bin/kmeans ./images/input.jpg ./out/q7O0output5.jpg 5
make clean && make OPTIMIZE=O3
perf stat -e instructions,cycles,cache-references,cache-misses,branch-instructions,branch-misses ./build/bin/kmeans ./images/input.jpg ./out/q7O3output5.jpg 5