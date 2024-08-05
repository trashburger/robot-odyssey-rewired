#!/bin/sh
set -e
rm -Rf dist build
docker build .
image=$(docker build -q .)
container=$(docker create $image)
docker cp $container:/work/dist dist
docker rm $container

