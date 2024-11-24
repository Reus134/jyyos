#!/bin/bash
mkdir build
gcc -fsanitize=address -U_FORTIFY_SOURCE -g -shared -fPIC -O0 -o build/libco.so co.c

gcc -fsanitize=address -U_FORTIFY_SOURCE -g -O0 -o build/demo demo.c -L ./build -lco -Wl,-rpath=./build

#export LD_LIBRARY_PATH=~/learning_ws/jyyos/thread/labco/build:$LD_LIBRARY_PATH