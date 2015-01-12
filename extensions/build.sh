#!/bin/bash

set -e

platform=$(python -c 'import platform; print 64 if "64" in platform.machine() else 32')

if [[ $platform == "64" ]]; then
    make x64
else
    make
fi
