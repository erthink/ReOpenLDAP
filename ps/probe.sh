#!/bin/bash

REV=$1
shift
git checkout $REV && git reset --hard && $(dirname $0)/bisect-probe.sh "$@"
