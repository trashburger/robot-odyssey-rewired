#!/bin/sh
set -e
rm -Rf dist build
git submodule update --init --recursive
docker build .
image=$(docker build -q .)
container=$(docker create $image)
docker cp $container:/work/dist dist
docker cp $container:/work/build build
docker cp $container:/work/package-lock.json ./
docker rm $container

