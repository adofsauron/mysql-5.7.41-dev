#!/bin/bash

cd mysql-5.7.41

mkdir -p build
cd build

export CPPFLAGS="-DOPTIMIZER_TRACE"

cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DDOWNLOAD_BOOST=1 -DWITH_BOOST=/tmp 

make -j`nproc`
make install

cd ../..
