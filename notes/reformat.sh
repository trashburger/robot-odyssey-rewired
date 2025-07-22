#!/bin/sh
set -e
cd src

black --verbose $(find -name '*.py') &
prettier -w $(find -name '*.js') &
clang-format --verbose -i $(find -name '*.h' -o -name '*.cpp') &
wait

