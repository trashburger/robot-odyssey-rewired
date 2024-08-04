#!/bin/sh
set -e
rm -Rf dist build
docker build .
image=$(docker build -q .)
container=$(docker create $image)
docker cp $container:/home/user/dist dist
docker rm $container

