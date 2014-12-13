#ifndef WRAP_LIBSFQ_INCLUDE_ONCE_H__
#define WRAP_LIBSFQ_INCLUDE_ONCE_H__

#define SFQWL_ZH_UPDATE_LONG(hash_, key_, val_) \
	{ \
		zval* zval_ = NULL; \
		MAKE_STD_ZVAL(zval_); \
		ZVAL_LONG(zval_, val_); \
		zend_hash_update(hash_, key_, strlen(key_) + 1, &zval_, sizeof(zval_), NULL); \
	}

#define SFQWL_ZH_UPDATE_STRING(hash_, key_, val_) \
	{ \
		if (val_) { \
			zval* zval_ = NULL; \
			MAKE_STD_ZVAL(zval_); \
			ZVAL_STRING(zval_, val_, 1); \
			zend_hash_update(hash_, key_, strlen(key_) + 1, &zval_, sizeof(zval_), NULL); \
		} \
	}

#define SFQWL_ZH_UPDATE_BINARY(hash_, key_, val_, valsize_) \
	{ \
		if (val_ && valsize_) { \
			zval* zval_ = NULL; \
			MAKE_STD_ZVAL(zval_); \
			ZVAL_STRINGL(zval_, val_, valsize_, 1); \
			zend_hash_update(hash_, key_, strlen(key_) + 1, &zval_, sizeof(zval_), NULL); \
		} \
	}

#endif

