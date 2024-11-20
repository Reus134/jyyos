#!/bin/bash
mkdir build
gcc -g -shared -fPIC -o build/libco.so co.c

gcc -g -o build/demo demo.c -L ./build -lco -Wl,-rpath=./build

#export LD_LIBRARY_PATH=~/learning_ws/jyyos/thread/labco/build:$LD_LIBRARY_PATH