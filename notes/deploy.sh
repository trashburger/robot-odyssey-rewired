#!/bin/sh
set -e

git pull
git checkout master
git submodule update --init --recursive

npm install

make clean
make

# Might need to update server settings
rsync -avz notes/htaccess robotodyssey.online:~/robotodyssey.online/.htaccess

# Upload all the versioned resources
rsync -avz `ls dist/* | egrep -v '^dist/(index\.html|sw\.js)$'` robotodyssey.online:~/robotodyssey.online/

# Now the service worker (with its resource list)
rsync -avz dist/sw.js robotodyssey.online:~/robotodyssey.online/

# Finally the entry point
rsync -avz dist/index.html robotodyssey.online:~/robotodyssey.online/
