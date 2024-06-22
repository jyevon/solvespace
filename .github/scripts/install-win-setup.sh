#!/bin/sh -xe

curl -Lo is.exe https://jrsoftware.org/download.php/is.exe
start /wait is.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART
