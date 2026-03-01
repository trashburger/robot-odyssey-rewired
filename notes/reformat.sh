#!/bin/sh
set -e
cd packages

black --verbose $(find -name '*.py') &
prettier -w --print-width=120 $(find -name '*.ts' -o -name '*.ts') &
clang-format --verbose -i $(find -name '*.h' -o -name '*.c' -o -name '*.cpp' | grep -v build/) &
wait

