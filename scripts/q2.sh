#!/bin/bash

make clean && make OPTIMIZE=O3
./build/bin/kmeans ./images/input.jpg ./out/q2output5.jpg 5
./build/bin/kmeans ./images/input.jpg ./out/q2output10.jpg 10
./build/bin/kmeans ./images/input.jpg ./out/q2output20.jpg 20