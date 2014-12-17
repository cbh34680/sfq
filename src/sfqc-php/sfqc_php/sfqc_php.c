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

#include "php_sfqc_php.h"

#if HAVE_SFQC_PHP

#include <ext/standard/php_smart_str.h>
#include "../sfqc-php.h"
/* {{{ sfqc_php_functions[] */
zend_function_entry sfqc_php_functions[] = {
	PHP_FE(wrap_sfq_push       , wrap_sfq_push_arg_info)
	PHP_FE(wrap_sfq_takeout    , wrap_sfq_takeout_arg_info)
	{ NULL, NULL, NULL }
};
/* }}} */


/* {{{ sfqc_php_module_entry
 */
zend_module_entry sfqc_php_module_entry = {
	STANDARD_MODULE_HEADER,
	"sfqc_php",
	sfqc_php_functions,
	PHP_MINIT(sfqc_php),     /* Replace with NULL if there is nothing to do at php startup   */ 
	PHP_MSHUTDOWN(sfqc_php), /* Replace with NULL if there is nothing to do at php shutdown  */
	PHP_RINIT(sfqc_php),     /* Replace with NULL if there is nothing to do at request start */
	PHP_RSHUTDOWN(sfqc_php), /* Replace with NULL if there is nothing to do at request end   */
	PHP_MINFO(sfqc_php),
	PHP_SFQC_PHP_VERSION, 
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_SFQC_PHP
ZEND_GET_MODULE(sfqc_php)
#endif


/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(sfqc_php)
{
	REGISTER_LONG_CONSTANT("SFQ_RC_SUCCESS", SFQ_RC_SUCCESS, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SFQ_RC_W_NOELEMENT", SFQ_RC_W_NOELEMENT, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SFQ_RC_W_NOSPACE", SFQ_RC_W_NOSPACE, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SFQ_RC_W_ACCEPT_STOPPED", SFQ_RC_W_ACCEPT_STOPPED, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SFQ_RC_W_TAKEOUT_STOPPED", SFQ_RC_W_TAKEOUT_STOPPED, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SFQ_RC_FATAL_MIN", SFQ_RC_FATAL_MIN, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SFQ_PLT_NULLTERM", SFQ_PLT_NULLTERM, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SFQ_PLT_BINARY", SFQ_PLT_BINARY, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SFQ_PLT_CHARARRAY", SFQ_PLT_CHARARRAY, CONST_PERSISTENT | CONST_CS);

	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(sfqc_php)
{

	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(sfqc_php)
{
	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RSHUTDOWN_FUNCTION */
PHP_RSHUTDOWN_FUNCTION(sfqc_php)
{
	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(sfqc_php)
{
	php_printf("libsfq wrapper extension\n");
	php_info_print_table_start();
	php_info_print_table_row(2, "Version",PHP_SFQC_PHP_VERSION " (stable)");
	php_info_print_table_row(2, "Released", "2014-12-08");
	php_info_print_table_row(2, "CVS Revision", "$Id: $");
	php_info_print_table_row(2, "Authors", "cbh34680 'cbh34680@iret.co.jp' (lead)\n");
	php_info_print_table_end();
	/* add your stuff here */

}
/* }}} */


/* {{{ proto int wrap_sfq_push(string querootdir, string quename, array ioparam)
  libsfq:sfq_push() wrapper */
PHP_FUNCTION(wrap_sfq_push)
{

	const char * querootdir = NULL;
	int querootdir_len = 0;
	const char * quename = NULL;
	int quename_len = 0;
	zval * ioparam = NULL;
	HashTable * ioparam_hash = NULL;



	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssa/", &querootdir, &querootdir_len, &quename, &quename_len, &ioparam) == FAILURE) {
		return;
	}
	ioparam_hash = HASH_OF(ioparam);



	do {
		// start
			zval** entry = NULL;
			int hnelm = 0;
		
			int push_rc = SFQ_RC_UNKNOWN;
			struct sfq_value val;
		
		/* init */
			bzero(&val, sizeof(val));
		
			zend_hash_internal_pointer_reset(ioparam_hash);
		
			hnelm = zend_hash_num_elements(ioparam_hash);
			if (hnelm == 0)
			{
				RETURN_FALSE;
			}
		
			while (zend_hash_get_current_data(ioparam_hash, (void**)&entry) == SUCCESS)
			{
				char* str_key = NULL;
				ulong num_key = 0;
				int hkey_type = 0;
				int hval_type = 0;
		
			// 要素のキーを取得
				hkey_type = zend_hash_get_current_key(ioparam_hash, &str_key, &num_key, 0);
		
				if (hkey_type != HASH_KEY_IS_STRING)
				{
					goto CONTINUE_LABEL;
				}
		
				if (! str_key)
				{
					goto CONTINUE_LABEL;
				}
		
			// 文字列要素の値を取得
		
				hval_type = Z_TYPE_PP(entry);
		
				switch (hval_type)
				{
					case IS_STRING:
					{
						char* str_val = Z_STRVAL_PP(entry);
		
						if (strcasecmp("execpath", str_key) == 0)
						{
							val.execpath = str_val;
						}
						else if (strcasecmp("execargs", str_key) == 0)
						{
							val.execargs = str_val;
						}
						else if (strcasecmp("metatext", str_key) == 0)
						{
							val.metatext = str_val;
						}
						else if (strcasecmp("soutpath", str_key) == 0)
						{
							val.soutpath = str_val;
						}
						else if (strcasecmp("serrpath", str_key) == 0)
						{
							val.serrpath = str_val;
						}
						else if (strcasecmp("payload", str_key) == 0)
						{
							if (! val.payload_size)
							{
								val.payload_size = Z_STRLEN_PP(entry);
							}
							val.payload = (sfq_byte*)str_val;
						}
						else if (strcasecmp("payload_size", str_key) == 0)
						{
							char* e = NULL;
							val.payload_size = (size_t)strtoul(str_val, &e, 0);
						}
						else if (strcasecmp("payload_type", str_key) == 0)
						{
							char* e = NULL;
							val.payload_type = (size_t)strtoul(str_val, &e, 0);
						}
		
						break;
					}
		
					case IS_LONG:
					{
						long long_val = Z_LVAL_PP(entry);
		
						if (strcasecmp("payload_size", str_key) == 0)
						{
							val.payload_size = (size_t)long_val;
						}
						else if (strcasecmp("payload_type", str_key) == 0)
						{
							val.payload_type = (payload_type_t)long_val;
						}
		
						break;
					}
				}
		
		
		CONTINUE_LABEL:
				zend_hash_move_forward(ioparam_hash);
			}
		
			push_rc = sfq_push(querootdir, quename, &val);
		
			if (push_rc == SFQ_RC_SUCCESS)
			{
				char uuid_s[36 + 1] = "";
		
				uuid_unparse(val.uuid, uuid_s);
		/*
				const char hkey_uuid[] = "uuid";
				zval* z_uuid = NULL;
		
				MAKE_STD_ZVAL(z_uuid);
				ZVAL_STRING(z_uuid, uuid_s, 1);
		
				zend_hash_update(ioparam_hash, hkey_uuid, sizeof(hkey_uuid), &z_uuid, sizeof(*z_uuid), NULL);
		*/
			// uuid
				SFQWL_ZH_UPDATE_STRING(ioparam_hash, "uuid", uuid_s);
		
			// payload_size
		/*
		payload_size は push 前に算出している可能性があるので書き戻す
		*/
				SFQWL_ZH_UPDATE_LONG(ioparam_hash, "payload_size", val.payload_size);
			}
		
		/*
		val に設定されているものは、全て php のメモリ領域なので開放する必要がない
		
			sfq_free_value(&val);
		*/
		
			RETURN_LONG(push_rc);
		// end
	} while (0);
}
/* }}} wrap_sfq_push */


/* {{{ proto int wrap_sfq_takeout(string querootdir, string quename, array ioparam, string takeoutfunc)
  libsfq:sfq_takeout() wrapper */
PHP_FUNCTION(wrap_sfq_takeout)
{

	const char * querootdir = NULL;
	int querootdir_len = 0;
	const char * quename = NULL;
	int quename_len = 0;
	zval * ioparam = NULL;
	HashTable * ioparam_hash = NULL;
	const char * takeoutfunc = NULL;
	int takeoutfunc_len = 0;



	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssa/s", &querootdir, &querootdir_len, &quename, &quename_len, &ioparam, &takeoutfunc, &takeoutfunc_len) == FAILURE) {
		return;
	}
	ioparam_hash = HASH_OF(ioparam);



	do {
		// start
			zval** entry = NULL;
		
			int takeout_rc = SFQ_RC_UNKNOWN;
			struct sfq_value val;
		
		/* init */
			bzero(&val, sizeof(val));
		
			if (strcmp(takeoutfunc, "sfq_shift") == 0)
			{
				takeout_rc = sfq_shift(querootdir, quename, &val);
			}
			else if (strcmp(takeoutfunc, "sfq_pop") == 0)
			{
				takeout_rc = sfq_pop(querootdir, quename, &val);
			}
		
			if (takeout_rc == SFQ_RC_SUCCESS)
			{
		/*
		取り出した val を連想配列にコピー
		*/
				char uuid_s[36 + 1] = "";
				uuid_unparse(val.uuid, uuid_s);
		
			// id
				SFQWL_ZH_UPDATE_LONG(ioparam_hash, "id", val.id);
		
			// pushtime
				SFQWL_ZH_UPDATE_LONG(ioparam_hash, "pushtime", val.pushtime);
		
			// uuid
				SFQWL_ZH_UPDATE_STRING(ioparam_hash, "uuid", uuid_s);
		
			// execpath
				SFQWL_ZH_UPDATE_STRING(ioparam_hash, "execpath", val.execpath);
		
			// execargs
				SFQWL_ZH_UPDATE_STRING(ioparam_hash, "execargs", val.execargs);
		
			// metatext
				SFQWL_ZH_UPDATE_STRING(ioparam_hash, "metatext", val.metatext);
		
			// soutpath
				SFQWL_ZH_UPDATE_STRING(ioparam_hash, "soutpath", val.soutpath);
		
			// serrpath
				SFQWL_ZH_UPDATE_STRING(ioparam_hash, "serrpath", val.serrpath);
		
			// payload_type
				SFQWL_ZH_UPDATE_LONG(ioparam_hash, "payload_type", val.payload_type);
		
			// payload_size
				SFQWL_ZH_UPDATE_LONG(ioparam_hash, "payload_size", val.payload_size);
		
			// payload
				SFQWL_ZH_UPDATE_BINARY(ioparam_hash, "payload", val.payload, val.payload_size);
			}
		
			sfq_free_value(&val);
		
			RETURN_LONG(takeout_rc);
		// end
	} while (0);
}
/* }}} wrap_sfq_takeout */

#endif /* HAVE_SFQC_PHP */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
