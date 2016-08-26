#!/bin/sh

cd ../src
find -name "*.h" -o -name "*.cc" | xargs clang-format-3.8 -style=file -i
