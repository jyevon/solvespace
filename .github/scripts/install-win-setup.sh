#!/bin/sh -xe

curl -Lo is.exe https://jrsoftware.org/download.php/is.exe
start /wait is.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /DIR:"C:\Program Files (x86)\Inno Setup"
PATH="$PATH:/C/Program Files (x86)/Inno Setup"
