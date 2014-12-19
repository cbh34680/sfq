#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

rm -rf bin
mkdir bin

javac -deprecation -encoding UTF-8 -d bin -sourcepath src src/*.java
javah -cp bin -d src jp.co.iret.sfq.SFQueueClientLocal

#
pcp=$(readlink -f ../../lib/pkgconfig)

copt="$(pkg-config $pcp/libsfq.pc --cflags --libs) -I/usr/java/default/include/ -I/usr/java/default/include/linux/"

gcc -Wall -O2 -fPIC --shared -o libsfqc-jni.so src/SFQueueClientLocal.c $copt

jar cvfm sfqc-jni.jar manifest.cf -C bin/ .

exit 0

