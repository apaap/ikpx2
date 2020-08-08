#!/bin/bash
set -e
set -x

git submodule update --init --recursive

cd kissat
./configure --extreme --sat
make
cd -

printf "\n\033[32;1m **** build process completed successfully **** \033[0m\n"
