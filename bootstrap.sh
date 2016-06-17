#!/bin/bash
git clean -x -f -d && autoreconf --force --install --include=build
