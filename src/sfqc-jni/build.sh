#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

rm -rf jp

javac -g -deprecation -encoding UTF-8 -cp . -d . -sourcepath src src/*.java
javah -cp . -d src jp.co.iret.sfq.SFQueueClientLocal

#
pcp=$(readlink -f ../../lib/pkgconfig)

copt="$(pkg-config $pcp/libsfq.pc --cflags --libs) -I/usr/java/default/include/ -I/usr/java/default/include/linux/"

(
  cd src/
  gcc -Wall -O2 -fPIC --shared -o libsfqc-jni.so SFQueueClientLocal.c $copt
)


exit 0

