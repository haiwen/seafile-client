#!/bin/sh

set -e

build_file="build.py"

print "WARNING: this file is deprecated."
print "Please use $build_file instead"

"$build_file" "$@"
