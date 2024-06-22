#!/bin/sh -ex
cd "$(dirname $0)"


mkdir -p "build/bin/RelWithDebInfo/"

if [ -f "windows/solvespace.exe" ]
then
    cp "windows/solvespace.exe" "build/bin/RelWithDebInfo/"
fi

if [ -f "windows-openmp/solvespace-openmp.exe" ]
then
    cp "windows-openmp/solvespace-openmp.exe" "build/bin/RelWithDebInfo/"
fi

"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" solvespace.iss