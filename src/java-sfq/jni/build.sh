#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

rm -rf jp
javac -g -verbose -deprecation -encoding UTF-8 -cp . -d . -sourcepath src src/*.java

exit 0

