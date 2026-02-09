#!/bin/bash

if [ -d "./out" ]; then
    rm -rf ./out
fi

mkdir -p ./out

source ./scripts/q2.sh > ./out/q2.txt
source ./scripts/q3.sh > ./out/q3.txt
source ./scripts/q4.sh > ./out/q4.txt