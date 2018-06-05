#!/bin/sh
set -e
git pull
git submodule update --init --recursive
npm install
make clean
make
rsync -avz `ls dist/* | grep -v \.html$` robotodyssey.online:~/robotodyssey.online/
rsync -avz dist/*.html robotodyssey.online:~/robotodyssey.online/
