#!/bin/sh -ex
cd "$(dirname $0)"

if [ ! -d executables/ ]
then
    mkdir executables
fi

if [ -n "$(find executables/ -maxdepth 0 -empty)" ]
then
   echo "Please place the compiled .exe files in executables/ first"
   exit -1
fi


if ! command -v ISCC &> /dev/null
then
    PATH="$PATH:/C/Program Files (x86)/Inno Setup 6"
fi

ISCC solvespace.iss

exes=(Output/*.exe)
setup_name=$(basename ${exes[0]})
echo "::set-output name=setup_name::$setup_name"