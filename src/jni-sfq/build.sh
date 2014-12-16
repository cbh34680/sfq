#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

rm -rf jp c-header

javac -g -deprecation -encoding UTF-8 -cp . -d . -sourcepath src src/*.java
javah -cp . -d c-src jp.co.iret.sfq.SFQueueClientLocal

exit 0

