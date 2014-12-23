#!/bin/bash

# Strip profiles and comments from png files.
# This can avoid libpng warnings.
# See https://wiki.archlinux.org/index.php/Libpng_errors

[[ -x `which convert` ]] || {
    echo
    echo "Please install ImageMagick tool first".
    echo
    echo "On ubuntu: apt-get install imagemagick"
    echo "On macos: port install ImageMagick"
    echo
}

TOP_DIR=$(python -c "import os; print os.path.dirname(os.path.realpath('$0'))")/..

find $TOP_DIR -type f -name \*.png -exec convert -strip {} {} \;
