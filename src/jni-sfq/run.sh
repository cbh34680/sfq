#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

java -cp . jp.co.iret.sfq.SFQueue

exit 0

