#!/bin/bash
rm -rf tests/testrun/*
git clean -x -f -d -e tests/testrun && autoreconf --force --install --include=build
git checkout build/libltdl/loaders/preopen.c
