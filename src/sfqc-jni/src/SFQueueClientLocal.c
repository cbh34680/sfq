#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sfq.h>

#include "jp_co_iret_sfq_SFQueueClientLocal.h"

/*
===
# プリミティブ型

	Java のデータ型		ネイティブ型		説明
	boolean			jboolean		unsigned 8 ビット
	byte			jbyte			signed 8 ビット
	char			jchar			unsigned 16 ビット
	short			jshort			signed 16 ビット
	int			jint			signed 32 ビット
	long			jlong			signed 64 ビット
	float			jfloat			32 ビット
	double			jdouble			64 ビット
	void			void			なし

	次の定義は、利便性のため提供されています。

		#define JNI_FALSE  0 
		#define JNI_TRUE   1 

	jsize 整数型は、基本的な添字およびサイズを記述するために使用されます。

		typedef jint jsize; 

===
# メソッドのシグニチャ表示

	[devuser@t-saq src]$ javap -s java.lang.String
	...
	  public int offsetByCodePoints(int, int);
	    descriptor: (II)I

	  void getChars(char[], int);
	    descriptor: ([CI)V

	  public void getChars(int, int, char[], int);
	    descriptor: (II[CI)V

	  public void getBytes(int, int, byte[], int);
	    descriptor: (II[BI)V
	...

===
# 型のシグニチャー
	型のシグニチャー		Java のデータ型
	------------------------------------------------------
	 Z				boolean
	 B				byte
	 C				char
	 S				short
	 I				int
	 J				long
	 F				float
	 D				double
	 L fully-qualified-class ;	完全指定のクラス
	 [ type				type[]
	 ( arg-types ) ret-type		メソッドの型


	たとえば、次の Java メソッドには、

		long f (int n, String s, int[] arr); 

	次のような型のシグニチャーがあります。

		(ILjava/lang/String;[I)J 

===
*/

struct java_types
{
	jclass clzInteger;
	jclass clzLong;
	jclass clzString;
	jclass clzHashMap;
	jclass prmByteArray;

	jmethodID mid_clzInteger_intValue;
	jmethodID mid_clzLong_longValue;
	jmethodID mid_clzLong_init;

	jmethodID mid_clzHashMap_get;
	jmethodID mid_clzHashMap_put;
	jmethodID mid_clzHashMap_containsKey;
};

static struct java_types* allocJavaTypes(JNIEnv* jenv)
{
	bool succ = false;
	struct java_types* jt = NULL;

	jclass clzInteger = NULL;
	jclass clzLong = NULL;
	jclass clzString = NULL;
	jclass clzHashMap = NULL;
	jclass prmByteArray = NULL;

	jmethodID mid_clzInteger_intValue = NULL;
	jmethodID mid_clzLong_longValue = NULL;
	jmethodID mid_clzLong_init = NULL;

	jmethodID mid_clzHashMap_get = NULL;
	jmethodID mid_clzHashMap_put = NULL;
	jmethodID mid_clzHashMap_containsKey = NULL;

/* */
	clzInteger = (*jenv)->FindClass(jenv, "java/lang/Integer");
	if (! clzInteger)
	{
		goto EXIT_LABEL;
	}

	clzLong = (*jenv)->FindClass(jenv, "java/lang/Long");
	if (! clzLong)
	{
		goto EXIT_LABEL;
	}

	clzString = (*jenv)->FindClass(jenv, "java/lang/String");
	if (! clzString)
	{
		goto EXIT_LABEL;
	}

	clzHashMap = (*jenv)->FindClass(jenv, "java/util/HashMap");
	if (! clzHashMap)
	{
		goto EXIT_LABEL;
	}

	//
	// http://read.pudn.com/downloads194/sourcecode/java/914038/jcom224/cpp/VARIANT.cpp__.htm
	//
	prmByteArray = (*jenv)->FindClass(jenv, "[B");
	if (! prmByteArray)
	{
		goto EXIT_LABEL;
	}

/* */
	mid_clzInteger_intValue = (*jenv)->GetMethodID(jenv, clzInteger, "intValue", "()I");
	if (! mid_clzInteger_intValue)
	{
		goto EXIT_LABEL;
	}

	mid_clzLong_longValue = (*jenv)->GetMethodID(jenv, clzLong, "longValue", "()J");
	if (! mid_clzLong_longValue)
	{
		goto EXIT_LABEL;
	}

	mid_clzLong_init = (*jenv)->GetMethodID(jenv, clzLong, "<init>", "(J)V");
	if (! mid_clzLong_init)
	{
		goto EXIT_LABEL;
	}

	mid_clzHashMap_get = (*jenv)->GetMethodID(jenv, clzHashMap,
		"get", "(Ljava/lang/Object;)Ljava/lang/Object;");

	if (! mid_clzHashMap_get)
	{
		goto EXIT_LABEL;
	}

	mid_clzHashMap_put = (*jenv)->GetMethodID(jenv, clzHashMap,
		"put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

	if (! mid_clzHashMap_put)
	{
		goto EXIT_LABEL;
	}

	mid_clzHashMap_containsKey = (*jenv)->GetMethodID(jenv, clzHashMap,
		"containsKey", "(Ljava/lang/Object;)Z");

	if (! mid_clzHashMap_containsKey)
	{
		goto EXIT_LABEL;
	}


/* */
	jt = malloc(sizeof(struct java_types));
	bzero(jt, sizeof(struct java_types));

	jt->clzInteger			= clzInteger;
	jt->clzLong			= clzLong;
	jt->clzString			= clzString;
	jt->clzHashMap			= clzHashMap;
	jt->prmByteArray		= prmByteArray;

	jt->mid_clzInteger_intValue	= mid_clzInteger_intValue;
	jt->mid_clzLong_longValue	= mid_clzLong_longValue;
	jt->mid_clzLong_init		= mid_clzLong_init;

	jt->mid_clzHashMap_get		= mid_clzHashMap_get;
	jt->mid_clzHashMap_put		= mid_clzHashMap_put;
	jt->mid_clzHashMap_containsKey	= mid_clzHashMap_containsKey;

	succ = true;

EXIT_LABEL:

	if (! succ)
	{
		if (clzInteger)
		{
			(*jenv)->DeleteLocalRef(jenv, clzInteger);
		}

		if (clzLong)
		{
			(*jenv)->DeleteLocalRef(jenv, clzLong);
		}

		if (clzString)
		{
			(*jenv)->DeleteLocalRef(jenv, clzString);
		}

		if (clzHashMap)
		{
			(*jenv)->DeleteLocalRef(jenv, clzHashMap);
		}

		if (prmByteArray)
		{
			(*jenv)->DeleteLocalRef(jenv, prmByteArray);
		}
	}

	return jt;
}

static void releaseJavaTypes(JNIEnv* jenv, struct java_types* jt)
{
	if (! jt)
	{
		return;
	}

	if (jt->clzInteger)
	{
		(*jenv)->DeleteLocalRef(jenv, jt->clzInteger);
	}

	if (jt->clzLong)
	{
		(*jenv)->DeleteLocalRef(jenv, jt->clzLong);
	}

	if (jt->clzString)
	{
		(*jenv)->DeleteLocalRef(jenv, jt->clzString);
	}

	if (jt->clzHashMap)
	{
		(*jenv)->DeleteLocalRef(jenv, jt->clzHashMap);
	}

	if (jt->prmByteArray)
	{
		(*jenv)->DeleteLocalRef(jenv, jt->prmByteArray);
	}

	free(jt);
}

jobject hashGetJObject(JNIEnv* jenv, jobject jhashmap, struct java_types* jt, const char* ckey)
{
	jobject jval = NULL;
	jstring jkey = NULL;

	jkey = (*jenv)->NewStringUTF(jenv, ckey);
	if (jkey)
	{
		jboolean jbool_ = (*jenv)->CallBooleanMethod(jenv, jhashmap, jt->mid_clzHashMap_containsKey, jkey);
		if (jbool_)
		{
			jval = (jstring)(*jenv)->CallObjectMethod(jenv, jhashmap, jt->mid_clzHashMap_get, jkey);
		}
		(*jenv)->DeleteLocalRef(jenv, jkey);
	}

	return jval;
}

char* hashGetCString(JNIEnv* jenv, jobject jhashmap, struct java_types* jt, const char* ckey)
{
	char* cval = NULL;
	jobject jobj = NULL;

	jobj = hashGetJObject(jenv, jhashmap, jt, ckey);
	if (jobj)
	{
		if ((*jenv)->IsInstanceOf(jenv, jobj, jt->clzString))
		{
			jstring jstr = (jstring)jobj;
			const char* cstr = NULL;

			cstr = (*jenv)->GetStringUTFChars(jenv, jstr, NULL);
			if (cstr)
			{
				cval = strdup(cstr);

				(*jenv)->ReleaseStringUTFChars(jenv, jstr, cstr);
			}
		}
		(*jenv)->DeleteLocalRef(jenv, jobj);
	}

	return cval;
}

long hashGetCLong(JNIEnv* jenv, jobject jhashmap, struct java_types* jt, const char* ckey)
{
	long cval = 0;
	jobject jobj = NULL;

	jobj = hashGetJObject(jenv, jhashmap, jt, ckey);
	if (jobj)
	{
		if ((*jenv)->IsInstanceOf(jenv, jobj, jt->clzLong))
		{
			cval = (*jenv)->CallLongMethod(jenv, jobj, jt->mid_clzLong_longValue);
		}
		else if ((*jenv)->IsInstanceOf(jenv, jobj, jt->clzInteger))
		{
			cval = (long)(*jenv)->CallIntMethod(jenv, jobj, jt->mid_clzInteger_intValue);
		}
		(*jenv)->DeleteLocalRef(jenv, jobj);
	}

	return cval;
}

void hashPutCString(JNIEnv* jenv, jobject jhashmap, struct java_types* jt, const char* ckey, const char* cval)
{
	jstring jkey = NULL;
	jstring jval = NULL;

	if (ckey && cval)
	{
		jkey = (*jenv)->NewStringUTF(jenv, ckey);
		jval = (*jenv)->NewStringUTF(jenv, cval);

		(*jenv)->CallObjectMethod(jenv, jhashmap, jt->mid_clzHashMap_put, jkey, jval);

		(*jenv)->DeleteLocalRef(jenv, jkey);
		(*jenv)->DeleteLocalRef(jenv, jval);
	}
}

void hashPutCLong(JNIEnv* jenv, jobject jhashmap, struct java_types* jt, const char* ckey, long cval)
{
	jstring jkey = NULL;
	jobject jval = NULL;

	if (ckey)
	{
		jkey = (*jenv)->NewStringUTF(jenv, ckey);
		jval = (*jenv)->NewObject(jenv, jt->clzLong, jt->mid_clzLong_init, cval);

		(*jenv)->CallObjectMethod(jenv, jhashmap, jt->mid_clzHashMap_put, jkey, jval);

		(*jenv)->DeleteLocalRef(jenv, jkey);
		(*jenv)->DeleteLocalRef(jenv, jval);
	}
}

void hashPutCBinary(JNIEnv* jenv, jobject jhashmap, struct java_types* jt, const char* ckey,
	const sfq_byte* cbarr, size_t cbarrsize)
{
	jstring jkey = NULL;
	jbyteArray jval = NULL;

	if (ckey && cbarr && cbarrsize)
	{
		jkey = (*jenv)->NewStringUTF(jenv, ckey);
		jval = (*jenv)->NewByteArray(jenv, cbarrsize);

		(*jenv)->SetByteArrayRegion(jenv, jval, 0, cbarrsize, (jbyte*)cbarr);
		(*jenv)->CallObjectMethod(jenv, jhashmap, jt->mid_clzHashMap_put, jkey, jval);

		(*jenv)->DeleteLocalRef(jenv, jkey);
		(*jenv)->DeleteLocalRef(jenv, jval);
	}
}

#ifdef __GNUC__
	#define STR_J2C(jstr_) \
		({ \
			const char* cstr_ = NULL; \
			if ( (jstr_) ) { \
				const char* tmp_ = (*jenv)->GetStringUTFChars(jenv, (jstr_), NULL); \
				if (tmp_) { \
					cstr_ = sfq_stradup(tmp_); \
					(*jenv)->ReleaseStringUTFChars(jenv, (jstr_), tmp_); \
				} \
			} \
			cstr_; \
		})
#endif

JNIEXPORT jint JNICALL Java_jp_co_iret_sfq_SFQueueClientLocal_wrap_1sfq_1push
  (JNIEnv* jenv, jobject jthis, jstring jquerootdir, jstring jquename, jobject jhashmap)
{
	int push_rc = SFQ_RC_UNKNOWN;

	const char* querootdir = NULL;
	const char* quename = NULL;

	struct java_types* jt = NULL;
	struct sfq_value val;

	jobject jobj_payload = NULL;


/* */
	bzero(&val, sizeof(val));

/* */
	jt = allocJavaTypes(jenv);
	if (! jt)
	{
		goto EXIT_LABEL;
	}

	querootdir = STR_J2C(jquerootdir);
	quename = STR_J2C(jquename);

	val.eworkdir = hashGetCString(jenv, jhashmap, jt, "eworkdir");
	val.execpath = hashGetCString(jenv, jhashmap, jt, "execpath");
	val.execargs = hashGetCString(jenv, jhashmap, jt, "execargs");
	val.metatext = hashGetCString(jenv, jhashmap, jt, "metatext");
	val.soutpath = hashGetCString(jenv, jhashmap, jt, "soutpath");
	val.serrpath = hashGetCString(jenv, jhashmap, jt, "serrpath");

	jobj_payload = hashGetJObject(jenv, jhashmap, jt, "payload");
	if (jobj_payload)
	{
		if ((*jenv)->IsInstanceOf(jenv, jobj_payload, jt->prmByteArray))
		{
			jbyteArray jbarr = (jbyteArray)jobj_payload;
			jsize cbarrlen = 0;

			cbarrlen = (*jenv)->GetArrayLength(jenv, jbarr);
			if (cbarrlen > 0)
			{
				jbyte* cbarr = NULL;

				cbarr = (*jenv)->GetByteArrayElements(jenv, jbarr, NULL);
				if (cbarr)
				{
					sfq_byte* payload = NULL;

/*
"+ 1" はいらないけど、なんとなく
*/
					payload = malloc(cbarrlen + 1);
					if (payload)
					{
						memcpy(payload, cbarr, cbarrlen);
						payload[cbarrlen] = '\0';

						val.payload = payload;
						val.payload_type = (SFQ_PLT_BINARY);
						val.payload_size = (size_t)cbarrlen;
					}

					(*jenv)->ReleaseByteArrayElements(jenv, jbarr, cbarr, 0);
				}
			}
		}
		else if ((*jenv)->IsInstanceOf(jenv, jobj_payload, jt->clzString))
		{
			jstring jstr = (jstring)jobj_payload;
			const char* cstr = NULL;

			cstr = (*jenv)->GetStringUTFChars(jenv, jstr, NULL);
			if (cstr)
			{
				char* payload = NULL;

				payload = strdup(cstr);
				if (payload)
				{
					val.payload = (sfq_byte*)payload;
					val.payload_type = (SFQ_PLT_CHARARRAY | SFQ_PLT_NULLTERM);
					val.payload_size = strlen(payload) + 1;
				}

				(*jenv)->ReleaseStringUTFChars(jenv, jstr, cstr);
			}
		}

		(*jenv)->DeleteLocalRef(jenv, jobj_payload);
		jobj_payload = NULL;
	}

	push_rc = sfq_push(querootdir, quename, &val);

	if (push_rc == SFQ_RC_SUCCESS)
	{
		char uuid_s[36 + 1] = "";

		uuid_unparse(val.uuid, uuid_s);

		hashPutCString(jenv, jhashmap, jt, "uuid", uuid_s);
	}


EXIT_LABEL:

	if (jobj_payload)
	{
		(*jenv)->DeleteLocalRef(jenv, jobj_payload);
		jobj_payload = NULL;
	}

	sfq_free_value(&val);

	releaseJavaTypes(jenv, jt);

	return push_rc;
}

JNIEXPORT jint JNICALL Java_jp_co_iret_sfq_SFQueueClientLocal_warp_1sfq_1takeout
  (JNIEnv* jenv, jobject jthis, jstring jquerootdir, jstring jquename, jobject jhashmap, jstring jtakeoutfunc)
{
	int takeout_rc = SFQ_RC_UNKNOWN;

	const char* querootdir = NULL;
	const char* quename = NULL;
	const char* takeoutfunc = NULL;

	struct java_types* jt = NULL;
	struct sfq_value val;

/* init */
	bzero(&val, sizeof(val));

	jt = allocJavaTypes(jenv);
	if (! jt)
	{
		goto EXIT_LABEL;
	}

	querootdir = STR_J2C(jquerootdir);
	quename = STR_J2C(jquename);
	takeoutfunc = STR_J2C(jtakeoutfunc);

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
		char uuid_s[36 + 1] = "";

		uuid_unparse(val.uuid, uuid_s);

	// id
		hashPutCLong(jenv, jhashmap, jt, "id", (long)val.id);

	// pushtime
		hashPutCLong(jenv, jhashmap, jt, "pushtime", (long)val.pushtime);

	// uuid
		hashPutCString(jenv, jhashmap, jt, "uuid", uuid_s);

	// eworkdir
		hashPutCString(jenv, jhashmap, jt, "eworkdir", val.eworkdir);

	// execpath
		hashPutCString(jenv, jhashmap, jt, "execpath", val.execpath);

	// execargs
		hashPutCString(jenv, jhashmap, jt, "execargs", val.execargs);

	// metatext
		hashPutCString(jenv, jhashmap, jt, "metatext", val.metatext);

	// soutpath
		hashPutCString(jenv, jhashmap, jt, "soutpath", val.soutpath);

	// serrpath
		hashPutCString(jenv, jhashmap, jt, "serrpath", val.serrpath);

	// payload
		if (val.payload_size && val.payload_type)
		{
			if (val.payload_type & SFQ_PLT_CHARARRAY)
			{
				char* payload = NULL;

				if (val.payload_type & SFQ_PLT_NULLTERM)
				{
					payload = (char*)val.payload;
				}
				else
				{
					payload = sfq_strandup((char*)val.payload, val.payload_size);
				}

				hashPutCString(jenv, jhashmap, jt, "payload", payload);
			}
			else if (val.payload_type & SFQ_PLT_BINARY)
			{
				hashPutCBinary(jenv, jhashmap, jt, "payload", val.payload, val.payload_size);
			}
		}
	}

EXIT_LABEL:

	sfq_free_value(&val);

	releaseJavaTypes(jenv, jt);

	return takeout_rc;
}

