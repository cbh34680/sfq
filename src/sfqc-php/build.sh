#!/bin/sh

unalias -a
cd $(dirname $(readlink -f "$0"))

rm -rf sfqc_php

cat << EOF > ../../lib/pkgconfig/libsfq.pc
prefix=$(readlink -f ../..)
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: sfq
Description: Library for manipulating queue data
Version: 0.11
Libs: -L\${libdir} -lcap -luuid -lsfq
Cflags: -I\${includedir} -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
EOF

pcp=$(readlink -f ../../lib/pkgconfig)
export PKG_CONFIG_PATH="$pcp"
echo "PKG_CONFIG_PATH=[$PKG_CONFIG_PATH]"

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
pecl-gen -f sfqc-php.xml 2>&1 \
\
| grep -v '^Runtime Notice: ' \
| grep -v '^Notice: '

(
  cd sfqc_php/ || {
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

  ./configure --enable-sfqc_php
  [ $? != 0 ] && exit 1

  #
  # pecl-gen が参照渡しできない (bug?) ため、生成したヘッダを直接編集する
  #
  cp -p php_sfqc_php.h php_sfqc_php.h.orig
  sed -i 's/\b0, ioparam\b/1, ioparam/g' php_sfqc_php.h
  sed -i 's/wrap_sfq_push_arg_info NULL$/wrap_sfq_push_arg_info first_arg_force_ref/' php_sfqc_php.h

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

  #cp -p modules/sfqc_php.so ..

  cat << EOF > ../root.sh
#!/bin/sh
pushd $(readlink -f .)
make install
echo 'extension=sfqc_php.so' > /etc/php.d/sfqc_php.ini
EOF
  chmod a+x ../root.sh

  cat << EOF
#
# ALL DONE.
#
# Don't forget to run following (as root)
#
#  1. (cd $(readlink -f .); make install)
#  2. echo 'extension=sfqc_php.so' > /etc/php.d/sfqc_php.ini
#  3. php -m | grep sfqc_php
#
# OR
#
#  $(readlink -f ..)/root.sh
#
EOF
)

exit $?

