#!/bin/sh
set -e
git pull
git submodule update --init --recursive
npm install
make clean
make
scp `ls dist/* | grep -v \.html$; ls dist/index.html` robotodyssey.online:~/robotodyssey.online/
scp notes/htaccess robotodyssey.online:~/robotodyssey.online/.htaccess
