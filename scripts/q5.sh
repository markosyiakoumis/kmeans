#!/bin/bash

make clean && make OPTIMIZE=O0
objdump -d ./build/bin/kmeans
make clean && make OPTIMIZE=O3
objdump -d ./build/bin/kmeans