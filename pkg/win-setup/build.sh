#!/bin/sh -ex
# USAGE: ./build.sh [<version>]

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


version="$1"
if [ -n "$version" ]
then
    version="${version#v}"
else
    rev="$GITHUB_SHA"
    if [ -z "$rev" ]
    then
        rev=HEAD
    fi
    
    version=$(git rev-parse --short "$rev")
fi

if ! command -v ISCC &> /dev/null
then
    PATH="$PATH:/C/Program Files (x86)/Inno Setup 6"
fi

APP_VERSION=$version ISCC solvespace.iss
