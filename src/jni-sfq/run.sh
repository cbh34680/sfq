#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

export LD_LIBRARY_PATH="./src:${LD_LIBRARY_PATH}"

#ulimit -c unlimited
java -cp . jp.co.iret.sfq.SFQueue

exit 0

