#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

export LD_LIBRARY_PATH="../..:${LD_LIBRARY_PATH}"

java -cp .:../../sfqc-jni.jar test.TestMain
#java -cp .:../../sfqc-jni.jar test.TestMain2

exit 0

