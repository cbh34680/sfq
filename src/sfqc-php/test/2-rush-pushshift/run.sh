#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

sfqc-clear

for i in {1..200000} ;
do
  cat << EOF
# ------------------------------------------------------------------
#
# loop [i = ${i}]
#
EOF

  php main.php nowait &
  php_pid=$!

  #sleepsec=$(( 3 + $i ))
  sleepsec=5

  echo "sleep ${sleepsec} sec"

  sleep $sleepsec

  #echo "kill pid=${php_pid}"
  #kill $php_pid

  #pkill -f main.php

  echo "kill ppid=${php_pid}"
  pkill -P $php_pid

done

exit 0

