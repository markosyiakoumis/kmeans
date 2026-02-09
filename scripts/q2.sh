#!/bin/bash

make clean && make OPTIMIZE=O3
time ./build/bin/kmeans ./images/input.jpg ./out/q2output5.jpg 5
time ./build/bin/kmeans ./images/input.jpg ./out/q2output10.jpg 10
time ./build/bin/kmeans ./images/input.jpg ./out/q2output20.jpg 20