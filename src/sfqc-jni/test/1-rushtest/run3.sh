#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

trap "pkill -f java" EXIT

export LD_LIBRARY_PATH="../..:${LD_LIBRARY_PATH}"

#rm -rf /var/tmp/jni1
#rm -rf /var/tmp/jni2

#sfqc-init -N jni1
#sfqc-init -N jni2

for i in {1..100000} ;
do
  echo "========== BIG LOOP $i =========="

  java -cp .:../../sfqc-jni.jar test.TestMain3 &
  jpid=$!

  echo "sleep 15 sec"
  sleep 15

  echo "kill pid=${jpid}"
  kill -15 $jpid

done

exit 0

