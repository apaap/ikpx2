#!/bin/bash

# Usage: ./recompile.sh [--rule RULESTRING]

rulearg=`echo "$@" | grep -o "\\-\\-rule [^ ]*" | sed "s/\\-\\-rule\\ //"`

set -e

echo "Updating submodules..."
git submodule update --init --recursive

if [ -f "kissat/build/libkissat.a" ]; then
echo "libkissat.a already detected"
else
echo "Building kissat solver..."
cd kissat
./configure --extreme --sat
cd build
make libkissat.a
cd ../..
fi

echo "Configuring lifelib..."
if command -v "python3" &>/dev/null; then
    echo "Using $(which python3) to configure lifelib..."
    python3 mkparams.py $rulearg
else
    echo "Using $(which python) to configure lifelib..."
    python mkparams.py $rulearg
fi

echo "Compiling ikpx2..."
g++ -O3 -Wall -Wextra -march=native --std=c++11 -Lkissat/build src/main.cpp -lkissat -pthread -o ikpx2

printf "\n\033[32;1m **** build process completed successfully **** \033[0m\n"
