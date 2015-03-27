#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

rm -rf bin
mkdir bin

cat << EOF > ../../lib/pkgconfig/libsfq.pc
prefix=$(readlink -f ../..)
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: sfq
Description: Library for manipulating queue data
Version: 0.11
Libs: -L\${libdir} -lcap -luuid -lsfq
Cflags: -I\${includedir} -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
EOF

#
SRCDIR=prjroot/src

javac -deprecation -encoding UTF-8 -d bin ${SRCDIR}/jp/co/iret/sfq/*.java
javah -cp bin -d ${SRCDIR} jp.co.iret.sfq.SFQueueClientLocal

#
pcp=$(readlink -f ../../lib/pkgconfig)

copt="$(pkg-config $pcp/libsfq.pc --cflags --libs) -I/usr/java/default/include/ -I/usr/java/default/include/linux/"

gcc -Wall -O2 -fPIC --shared -o libsfqc-jni.so ${SRCDIR}/SFQueueClientLocal.c $copt

jar cvfm sfqc-jni.jar manifest.cf -C bin/ .

exit 0

