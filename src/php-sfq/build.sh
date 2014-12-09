#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

[ ! -d wrap_libsfq/ ] && mkdir wrap_libsfq/
rm -rf wrap_libsfq/*

pcp=$(readlink -f ../../lib/pkgconfig)
export PKG_CONFIG_PATH="$pcp"

which pecl-gen 2> /dev/null
if [ $? != 0 ] ;
then

  cat << EOF
#
# pecl-gen command not found, try the following command (as root)
#
# 1. pear install CodeGen_PECL
#
EOF

  exit 1
fi

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
    exit 1
  }

  cat << EOF
#
# configure
#
EOF
  phpize
  [ $? != 0 ] && exit 1

  ./configure --enable-wrap_libsfq
  [ $? != 0 ] && exit 1

  # pecl-gen が参照渡しできない (bug?) ため、生成したヘッダを直接編集する
  sed -i 's/\b0, ioparam\b/1, ioparam/g' php_wrap_libsfq.h
  sed -i 's/wrap_sfq_push_arg_info NULL$/wrap_sfq_push_arg_info first_arg_force_ref/' php_wrap_libsfq.h

  cat << EOF
#
# make
#
EOF
  make
  if [ $? != 0 ] ;
  then
    cat << EOF
#
# If you find in make-error, such as:
# ----
# wrap_libsfq.c:24:1: error: unknown type name 'function_entry'
#  function_entry wrap_libsfq_functions[] = {
#  ^
# ----
# 
# try the following command (as root)
# 
# 1. cp -p /usr/share/pear/CodeGen/PECL/Extension.php /usr/share/pear/CodeGen/PECL/Extension.php.orig
# 2. patch -d /usr/share/pear/CodeGen/PECL/ < $(readlink -f ..)/Extension.php.patch
# 
EOF

    exit 1
  fi

  env NO_INTERACTION=1 make test
  [ $? != 0 ] && exit 1

  cat << EOF
#
# ALL DONE.
#
# Don't forget to run following (as root)
#
#  1. (cd $(readlink -f .); make install)
#  2. echo 'extension=wrap_libsfq.so' > /etc/php.d/wrap_libsfq.ini
#  3. php -m | grep wrap_libsfq
#
EOF
)

exit $?

