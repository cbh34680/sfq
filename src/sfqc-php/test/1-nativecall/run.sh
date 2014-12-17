#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

EXTSO='../../sfqc_php/modules/sfqc_php.so'
[ ! -f ${EXTSO} ] && echo "extension file not found" && exit 1

PHPOPT=''
[ ! -f /etc/php.d/sfqc_php.ini ] && PHPOPT="-d extension=${EXTSO}"

echo "PHPOPT=[$PHPOPT]"
echo

echo "* ls -l [input]"
ls -l ${EXTSO}
echo

echo "* test-push.php"
php ${PHPOPT} test-push.php $EXTSO
[ $? != 0 ] && echo "push fault" && exit 1
echo

echo "* test-takeout.php"
php ${PHPOPT} test-takeout.php
[ $? != 0 ] && echo "takeout fault" && exit 1
echo

echo "* ls -l [output]"
ls -l output-*.bin
echo

echo "* cmp input output"
cmp $EXTSO output-sfq_pop.bin
echo $?
cmp $EXTSO output-sfq_shift.bin
echo $?

rm -f output-*.bin

exit 0

