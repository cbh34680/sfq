/*
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: cbh34680 <cbh34680@iret.co.jp>                              |
   +----------------------------------------------------------------------+
*/

/* $ Id: $ */ 

#ifndef PHP_SFQC_PHP_H
#define PHP_SFQC_PHP_H

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>

#ifdef HAVE_SFQC_PHP
#define PHP_SFQC_PHP_VERSION "0.11.0"


#include <php_ini.h>
#include <SAPI.h>
#include <ext/standard/info.h>
#include <Zend/zend_extensions.h>
#ifdef  __cplusplus
} // extern "C" 
#endif
#include <sfq.h>
#ifdef  __cplusplus
extern "C" {
#endif

extern zend_module_entry sfqc_php_module_entry;
#define phpext_sfqc_php_ptr &sfqc_php_module_entry

#ifdef PHP_WIN32
#define PHP_SFQC_PHP_API __declspec(dllexport)
#else
#define PHP_SFQC_PHP_API
#endif

PHP_MINIT_FUNCTION(sfqc_php);
PHP_MSHUTDOWN_FUNCTION(sfqc_php);
PHP_RINIT_FUNCTION(sfqc_php);
PHP_RSHUTDOWN_FUNCTION(sfqc_php);
PHP_MINFO_FUNCTION(sfqc_php);

#ifdef ZTS
#include "TSRM.h"
#endif

#define FREE_RESOURCE(resource) zend_list_delete(Z_LVAL_P(resource))

#define PROP_GET_LONG(name)    Z_LVAL_P(zend_read_property(_this_ce, _this_zval, #name, strlen(#name), 1 TSRMLS_CC))
#define PROP_SET_LONG(name, l) zend_update_property_long(_this_ce, _this_zval, #name, strlen(#name), l TSRMLS_CC)

#define PROP_GET_DOUBLE(name)    Z_DVAL_P(zend_read_property(_this_ce, _this_zval, #name, strlen(#name), 1 TSRMLS_CC))
#define PROP_SET_DOUBLE(name, d) zend_update_property_double(_this_ce, _this_zval, #name, strlen(#name), d TSRMLS_CC)

#define PROP_GET_STRING(name)    Z_STRVAL_P(zend_read_property(_this_ce, _this_zval, #name, strlen(#name), 1 TSRMLS_CC))
#define PROP_GET_STRLEN(name)    Z_STRLEN_P(zend_read_property(_this_ce, _this_zval, #name, strlen(#name), 1 TSRMLS_CC))
#define PROP_SET_STRING(name, s) zend_update_property_string(_this_ce, _this_zval, #name, strlen(#name), s TSRMLS_CC)
#define PROP_SET_STRINGL(name, s, l) zend_update_property_stringl(_this_ce, _this_zval, #name, strlen(#name), s, l TSRMLS_CC)


PHP_FUNCTION(wrap_sfq_push);
#if (PHP_MAJOR_VERSION >= 5)
ZEND_BEGIN_ARG_INFO_EX(wrap_sfq_push_arg_info, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 3)
  ZEND_ARG_INFO(0, querootdir)
  ZEND_ARG_INFO(0, quename)
#if (PHP_MINOR_VERSION > 0)
  ZEND_ARG_ARRAY_INFO(1, ioparam, 1)
#else
  ZEND_ARG_INFO(1, ioparam)
#endif
ZEND_END_ARG_INFO()
#else /* PHP 4.x */
#define wrap_sfq_push_arg_info first_arg_force_ref
#endif

PHP_FUNCTION(wrap_sfq_takeout);
#if (PHP_MAJOR_VERSION >= 5)
ZEND_BEGIN_ARG_INFO_EX(wrap_sfq_takeout_arg_info, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 4)
  ZEND_ARG_INFO(0, querootdir)
  ZEND_ARG_INFO(0, quename)
#if (PHP_MINOR_VERSION > 0)
  ZEND_ARG_ARRAY_INFO(1, ioparam, 1)
#else
  ZEND_ARG_INFO(1, ioparam)
#endif
  ZEND_ARG_INFO(0, takeoutfunc)
ZEND_END_ARG_INFO()
#else /* PHP 4.x */
#define wrap_sfq_takeout_arg_info NULL
#endif

#ifdef  __cplusplus
} // extern "C" 
#endif

#endif /* PHP_HAVE_SFQC_PHP */

#endif /* PHP_SFQC_PHP_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
