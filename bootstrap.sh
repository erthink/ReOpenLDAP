#!/bin/bash
rm -rf tests/testrun/*
git clean -x -f -d -e tests/testrun && autoreconf --force --install --include=build
patch -p1 -i build/libltdl.patch
