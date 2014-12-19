#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

rm -rf test

javac -cp .:../../sfqc-jni.jar -d . TestMain.java
javac -cp .:../../sfqc-jni.jar -d . TestMain2.java

exit 0

