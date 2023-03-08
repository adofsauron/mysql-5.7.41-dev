#!/bin/bash

cd mysql-5.7.41

mkdir -p build

cd build

cmake .. 
make -j`nproc`
make install

cd ../..
