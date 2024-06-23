#!/bin/sh -ex
cd "$(dirname $0)"

REPO="../.."

mkdir -p "$REPO/build/bin/RelWithDebInfo/"

if [ -f "$REPO/windows/solvespace.exe" ]
then
    cp "$REPO/windows/solvespace.exe" "$REPO/build/bin/RelWithDebInfo/"
fi

if [ -f "$REPO/windows-openmp/solvespace-openmp.exe" ]
then
    cp "$REPO/windows-openmp/solvespace-openmp.exe" "$REPO/build/bin/RelWithDebInfo/"
fi

if ! command -v ISCC &> /dev/null
then
    PATH="$PATH:/C/Program Files (x86)/Inno Setup 6"
fi

ISCC solvespace.iss