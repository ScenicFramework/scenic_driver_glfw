#!/bin/sh

if command -v clang-format > /dev/null &2>1; then
    clang-format -i -style=file c_src/*.*
else
    echo "You'll need to install clang-format (available via apt for Debian and derivatives) for this to work."
    exit 1
fi