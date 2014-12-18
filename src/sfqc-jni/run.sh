#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

export LD_LIBRARY_PATH=".:${LD_LIBRARY_PATH}"

java -jar sfqc-jni.jar

exit 0

