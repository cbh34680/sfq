#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

EXTSO='../wrap_libsfq/modules/wrap_libsfq.so'
[ ! -f ${EXTSO} ] && echo "extension file not found" && exit 1

PHPOPT=''
[ ! -f /etc/php.d/wrap_libsfq.ini ] && PHPOPT="-d extension=${EXTSO}"

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

exit 0

