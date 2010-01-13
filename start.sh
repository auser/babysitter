#!/bin/sh

VERSION=$(cat VERSION | tr -d '\n')
PWD=$(dirname $0)
CONFIG=$1

erl -pa $PWD/ebin \
    -pa deps/*/ebin \
    -s reloader \
    -babysitter \
    -boot babysitter-$VERSION