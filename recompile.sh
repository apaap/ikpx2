#!/bin/bash
set -e
set -x

git submodule update --init --recursive

cd kissat
./configure --extreme --sat
cd build
make libkissat.a
cd ../..

g++ -O3 -Lkissat/build src/main.cpp -lkissat -o ikpx2

printf "\n\033[32;1m **** build process completed successfully **** \033[0m\n"
