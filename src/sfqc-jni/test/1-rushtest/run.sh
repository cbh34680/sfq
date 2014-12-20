#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

trap "pkil -f java" EXIT

export LD_LIBRARY_PATH="../..:${LD_LIBRARY_PATH}"

#ulimit -c unlimited
#rm -rf /var/tmp/noname
#sfqc-init

for i in {1..100000} ;
do
  echo "========== BIG LOOP $i =========="

  java -cp .:../../sfqc-jni.jar test.TestMain &
  jpid=$!

  echo "sleep 15 sec"
  sleep 15

  echo "kill pid=${jpid}"
  kill -15 $jpid

done

exit 0

