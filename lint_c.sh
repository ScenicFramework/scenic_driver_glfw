#!/bin/sh

if command -v cppcheck > /dev/null &2>1; then
    cppcheck c_src/*.* --enable=all
else
    echo "You'll need to install cppcheck (available via apt for Debian and derivatives) for this to work."
    exit 1
fi