dnl
dnl $ Id: $
dnl

PHP_ARG_ENABLE(sfqc_php, whether to enable sfqc_php functions,
[  --enable-sfqc_php         Enable sfqc_php support])

if test "$PHP_SFQC_PHP" != "no"; then

PHP_ARG_WITH(libsfq, whether libsfq is available,[  --with-libsfq[=DIR]  With libsfq support])


  
  if test -z "$PKG_CONFIG"
  then
	AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  fi
  if test "$PKG_CONFIG" = "no"
  then
	AC_MSG_ERROR([required utility 'pkg-config' not found])
  fi

  if ! $PKG_CONFIG --exists libsfq
  then
	AC_MSG_ERROR(['libsfq' not known to pkg-config])
  fi

  PHP_EVAL_INCLINE(`$PKG_CONFIG --cflags-only-I libsfq`)
  PHP_EVAL_LIBLINE(`$PKG_CONFIG --libs libsfq`, SFQC_PHP_SHARED_LIBADD)

  export OLD_CPPFLAGS="$CPPFLAGS"
  export CPPFLAGS="$CPPFLAGS $INCLUDES -DHAVE_LIBSFQ"
  AC_CHECK_HEADER([sfq.h], [], AC_MSG_ERROR('sfq.h' header not found))
  export CPPFLAGS="$OLD_CPPFLAGS"

  export OLD_CPPFLAGS="$CPPFLAGS"
  export CPPFLAGS="$CPPFLAGS $INCLUDES -DHAVE_SFQC_PHP"

  AC_MSG_CHECKING(PHP version)
  AC_TRY_COMPILE([#include <php_version.h>], [
#if PHP_VERSION_ID < 40000
#error  this extension requires at least PHP version 4.0.0
#endif
],
[AC_MSG_RESULT(ok)],
[AC_MSG_ERROR([need at least PHP 4.0.0])])

  export CPPFLAGS="$OLD_CPPFLAGS"


  PHP_SUBST(SFQC_PHP_SHARED_LIBADD)
  AC_DEFINE(HAVE_SFQC_PHP, 1, [ ])

  PHP_NEW_EXTENSION(sfqc_php, sfqc_php.c , $ext_shared)

fi

