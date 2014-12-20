#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

export LD_LIBRARY_PATH="../..:${LD_LIBRARY_PATH}"

rm -rf /var/tmp/jni2

sfqc-init -N jni2
[ $? != 0 ] && exit 1

java -cp .:../../sfqc-jni.jar test.TestMain2

exit 0

