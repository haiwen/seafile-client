#!/bin/bash
set -x
set -e
## TODO: use the correct version of seafile for each branch

brew up
brew install qt4

brew tap Chilledheart/seafile
brew install --HEAD libsearpc seafile
