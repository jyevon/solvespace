#!/bin/sh -xe

mkdir -p pkg/win-setup/executables/
cp windows/solvespace.exe windows-openmp/solvespace-openmp.exe pkg/win-setup/executables/

./pkg/win-setup/build.sh "$1"
