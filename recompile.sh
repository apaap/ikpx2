#!/bin/bash

# Usage: ./recompile.sh [--rule RULESTRING]

rulearg=`echo "$@" | grep -o "\\-\\-rule [^ ]*" | sed "s/\\-\\-rule\\ //"`

set -e

echo "Updating submodules..."
git submodule update --init --recursive

if [ -f "solvers/libkissat.a" ]; then
echo "libkissat.a already detected"
else
echo "Building kissat solver..."
cd kissat
./configure -s --extreme --sat
cd build
make libkissat.a
cd ../..
cp "kissat/build/libkissat.a" solvers
fi

if [ -f "solvers/libcadical.a" ]; then
echo "libcadical.a already detected"
else
echo "Building cadical solver..."
cd cadical
./configure -s --competition
cd build
make libcadical.a
cd ../..
cp "cadical/build/libcadical.a" solvers
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
g++ -O3 -Wall -Wextra -march=native --std=c++11 -Lsolvers src/main.cpp -lkissat -lcadical -pthread -o ikpx2 -g

printf "\n\033[32;1m **** build process completed successfully **** \033[0m\n"
