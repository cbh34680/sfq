#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

for i in {1..5} ;
do
  cat << EOF
# ------------------------------------------------------------------
#
# loop [i = ${i}]
#
EOF

  php main.php &
  php_pid=$!

  sleepsec=$(( 3 + $i ))
  echo "sleep ${sleepsec} sec"

  sleep $sleepsec

  #echo "kill pid=${php_pid}"
  #kill $php_pid

  #pkill -f main.php

  echo "kill ppid=${php_pid}"
  pkill -P $php_pid

done

exit 0

