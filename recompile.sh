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

if [ -f "solvers/libkissat3.a" ]; then
echo "libkissat.a already detected"
else
echo "Building kissat solver..."
cd kissat
./configure -s --extreme --sat
cd build
make libkissat.a
cd ../..
cp "kissat/build/libkissat.a" "solvers/libkissat3.a"
fi

if [ -f "solvers/libcadical3.a" ]; then
echo "libcadical.a already detected"
else
echo "Building cadical solver..."
cd cadical
./configure -s --competition
cd build
cat ../src/reap.hpp | sed 's/class/#include <stddef.h>\nclass/g' > modified.hpp
mv modified.hpp ../src/reap.hpp
make libcadical.a
git checkout -- ../src/reap.hpp
cd ../..
cp "cadical/build/libcadical.a" "solvers/libcadical3.a"
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
cp solvers/libkissat3.a solvers/libkissat.a
cp solvers/libcadical3.a solvers/libcadical.a

echo "Compiling ikpx2..."
sources="apgmera/includes/md5.cpp apgmera/includes/happyhttp.cpp src/main.cpp"
g++ -O3 -Wall -Wextra -march=native --std=c++11 -Lsolvers $sources -lkissat -lcadical -pthread -o ikpx2 -g

printf "\n\033[32;1m **** build process completed successfully **** \033[0m\n"
