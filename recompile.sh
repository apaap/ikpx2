#!/bin/bash

# Usage: ./recompile.sh [--rule RULESTRING]

rulearg=`echo "$@" | grep -o "\\-\\-rule [^ ]*" | sed "s/\\-\\-rule\\ //"`

if ((${#rulearg} == 0)); then
rulearg="b3s23"
echo "Rule unspecified; assuming b3s23."
fi

set -e

echo "Updating submodules..."
git submodule update --init --recursive

if [ -f "solvers/libkissat4.a" ]; then
echo "libkissat.a already detected"
else
echo "Building kissat solver..."
cd kissat_extras
./configure -s --compact --quiet --no-proofs --no-statistics
cd build
make libkissat.a
cd ../..
cp "kissat_extras/build/libkissat.a" "solvers/libkissat4.a"
fi

echo "Configuring lifelib..."
cd apgmera
if command -v "python3" &>/dev/null; then
    echo "Using $(which python3) to configure lifelib..."
    python3 mkparams.py $rulearg "ikpx2_stdin"
else
    echo "Using $(which python) to configure lifelib..."
    python mkparams.py $rulearg "ikpx2_stdin"
fi
cd ..

echo "Gathering latest library versions..."
cp solvers/libkissat4.a solvers/libkissat.a

echo "Compiling ikpx2..."
sources="apgmera/includes/md5.cpp apgmera/includes/happyhttp.cpp src/main.cpp"
g++ -O0 -Wall -Wextra -march=native --std=c++20 -Lsolvers $sources -lkissat -pthread -o ikpx2 -g

printf "\n\033[32;1m **** build process completed successfully **** \033[0m\n"
