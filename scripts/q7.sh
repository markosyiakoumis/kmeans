#!/bin/bash

make clean && make OPTIMIZE=O0
perf stat -e instructions,cycles,cache-references,cache-misses,branch-instructions,branch-misses ./build/bin/kmeans
make clean && make OPTIMIZE=O3
perf stat -e instructions,cycles,cache-references,cache-misses,branch-instructions,branch-misses ./build/bin/kmeans