#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

export LD_LIBRARY_PATH="./bin:${LD_LIBRARY_PATH}"

#ulimit -c unlimited
java -cp bin jp.co.iret.sfq.SFQueue

exit 0

