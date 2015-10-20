#!/bin/bash

REPONAME="librepo"
GITREPO="https://github.com/Tojaj/librepo.git"

MY_DIR="$( cd "$(dirname "$0")" ; pwd -P )"

pushd "$MY_DIR"
echo "Generating doc into $MY_DIR"

echo "Pulling changes"
git pull

TMP_DIR=$( mktemp -d )
echo "Using temporary directory: $TMP_DIR"

pushd "$TMP_DIR"
git clone --depth 1 --branch master --single-branch "$GITREPO"
cd "$REPONAME"
mkdir build
cd build
cmake ..
make
make doc
cp doc/python/*.html "$MY_DIR"/
cp doc/python/*.js "$MY_DIR"/
cp -r doc/python/_static "$MY_DIR"/
popd

echo "Removing: $TMP_DIR"
rm -rf "$TMP_DIR"

echo "Successfully finished"
echo "To push updated doc - commit changes and push them:"
echo "git commit -a -m \"Documentation update\""
echo "git push origin gh-pages"

popd
