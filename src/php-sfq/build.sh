#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

[ ! -d wrap_libsfq/ ] && mkdir wrap_libsfq/
rm -rf wrap_libsfq/*

pcp=$(readlink -f ../../lib/pkgconfig)
export PKG_CONFIG_PATH="$pcp"

cat << EOF
#
# pecl-gen
#
EOF
pecl-gen -f wrap_libsfq.xml 2>&1 \
\
| grep -v '^Runtime Notice: ' \
| grep -v '^Notice: '

(
  cd wrap_libsfq/ || {
    echo "change directory fault"
    exit
  }

  cat << EOF
#
# configure
#
EOF
  phpize
  [ $? != 0 ] && exit

  ./configure --enable-wrap_libsfq
  [ $? != 0 ] && exit

  # pecl-gen が参照渡しできない (bug?) ため、生成したヘッダを直接編集する
  sed -i 's/\b0, ioparam\b/1, ioparam/g' php_wrap_libsfq.h
  sed -i 's/wrap_sfq_push_arg_info NULL$/wrap_sfq_push_arg_info first_arg_force_ref/' php_wrap_libsfq.h

  cat << EOF
#
# make
#
EOF
  make
  [ $? != 0 ] && exit

  env NO_INTERACTION=1 make test
  [ $? != 0 ] && exit

  cat << EOF
#
# ALL DONE.
#
# Don't forget to run following (as root)
#
#  1. (cd $(readlink -f .)/wrap_libsfq/; make install)
#  2. echo 'extension=wrap_libsfq.so' > /etc/php.d/wrap_libsfq.ini
#
EOF
)

exit $?

