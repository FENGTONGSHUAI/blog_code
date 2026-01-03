#!/bin/sh
mkdir -p build && cd build
# cmake .. -DCMAKE_C_COMPILER=/usr/local/bin/gcc -DCMAKE_CXX_COMPILER=/usr/local/bin/g++
cmake ..
make -j$(nproc)
