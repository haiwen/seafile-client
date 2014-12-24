#!/bin/bash
set -x

brew up
brew install qt4

brew tap Chilledheart/seafile
brew install --HEAD libsearpc ccnet seafile
