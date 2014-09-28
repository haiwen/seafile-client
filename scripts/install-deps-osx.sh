#!/bin/bash
set -x
set -e

brew up
brew install qt5
brew link qt5

brew tap Chilledheart/seafile
brew install --HEAD libsearpc ccnet seafile
