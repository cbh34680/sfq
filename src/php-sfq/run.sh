#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

[ ! -f wrap_libsfq/modules/wrap_libsfq.so ] && echo "extension file not found" && exit 1

echo "* ls -l [input]"
ls -l wrap_libsfq/modules/wrap_libsfq.so
echo

echo "* test-push.php"
php -d extension=wrap_libsfq/modules/wrap_libsfq.so test-push.php
[ $? != 0 ] && echo "push fault" && exit 1
echo

echo "* test-takeout.php"
php -d extension=wrap_libsfq/modules/wrap_libsfq.so test-takeout.php
[ $? != 0 ] && echo "takeout fault" && exit 1
echo

echo "* ls -l [output]"
ls -l output-*.bin
echo

exit 0

