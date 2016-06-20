#!/bin/bash
git clean -x -f -d && autoreconf --force --install --include=build
git checkout build/libltdl/loaders/preopen.c
