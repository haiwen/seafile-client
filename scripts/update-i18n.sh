#!/usr/bin/env bash
set -e

PWD=$(dirname "${BASH_SOURCE[0]}")
cd $PWD/..

if [ ! -d .tx ]; then
    echo "please make sure this script is under the scripts directory!"
    exit -9
fi

if [ "$1" != "-v" ]; then
    QUIET_FLAGS="-silent"
fi

function regenerate_source() {
    if [ -f build.ninja ]; then
        cmake -DBUILD_SHIBBOLETH_SUPPORT=on
        ninja update-ts
    elif [ -f Makefile ]; then
        cmake -DBUILD_SHIBBOLETH_SUPPORT=on
        make update-ts
    else
        local SEAFILE_PROJECT="seafile-client.pro"
        rm -f $SEAFILE_PROJECT
        qmake -project -o $SEAFILE_PROJECT
        lrelease $SEAFILE_PROJECT $QUIET_FLAGS
        lupdate $SEAFILE_PROJECT $QUIET_FLAGS
        rm -f $SEAFILE_PROJECT
    fi
}

function git_diff() {
    set +e
    git diff i18n/seafile_en.ts
    set -e
}

function push_source() {
    tx push -s
    pushd fsplugin
    tx push -s
    popd
}

function pull_translations() {
    tx pull -a -f --minimum-perc=30
    pushd fsplugin
    tx pull -a -f --minimum-perc=80
    popd
}

echo "This script will help you to regenerate sources of translations, "
echo "update them to transifex and grab the latest translations from it"
echo
echo "========================================================================="
echo
echo "    Make sure you have review the source difference beforce pushing!"
echo "    Press Control+C once you want to abort this script"
echo
echo "========================================================================="
echo

echo "[0/4] Press any key to start"
read -s -n 1
echo "[1/4] Regenerating sources files"
regenerate_source
echo "[1/4] Regenerating sources files"

echo "[2/4] Press any key to view diff"
read -s -n 1
git_diff

echo "[3/4] Press any key to push source to transifix"
read -s -n 1
push_source

echo "[4/4] Press any key to pull translations"
read -s -n 1
pull_translations
echo "[4/4] All Done"
