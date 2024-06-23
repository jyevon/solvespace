#!/bin/sh -xe

mkdir -p pkg/win-setup/executables/
cp windows/solvespace.exe windows-openmp/solvespace-openmp.exe pkg/win-setup/executables/

./pkg/win-setup/build.sh "$1"

exes=(pkg/win-setup/Output/*.exe)
setup_name=$(basename ${exes[0]})
echo "::set-output name=setup_name::$setup_name"
