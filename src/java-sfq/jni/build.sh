#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

javac -cp . -d . src/*.java

exit 0

