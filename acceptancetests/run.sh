#!/bin/bash

pushd `dirname $0`
PYTHONPATH="../build/librepo/python/" lettuce
popd
