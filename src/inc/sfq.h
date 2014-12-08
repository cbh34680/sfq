#ifndef SFMQ_INCLUDE_H_ONCE__
#define SFMQ_INCLUDE_H_ONCE__

/* time_t */
#include <time.h>

/* ulong, ushort ... */
#include <sys/types.h>

#ifdef WIN32
	/* for msvc compile (compile only not link) */
	#define uuid_t	void*
#else
	/* yum install libuuid-devel */
	#include <uuid/uuid.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char sfq_uchar;
typedef unsigned char sfq_byte;

typedef sfq_uchar questate_t;
typedef sfq_uchar payload_type_t;

enum
{
	SFQ_RC_SUCCESS		= 0,

	SFQ_RC_UNKNOWN		= 11,

	SFQ_RC_NO_ELEMENT	= 51,
	SFQ_RC_NO_SPACE,
	SFQ_RC_ACCEPT_STOPPED,
	SFQ_RC_NOCHANGE_STATE,

	SFQ_RC_EA_FSIZESMALL	= 61,
	SFQ_RC_EA_EXISTQUEUE,
	SFQ_RC_EA_OPENFILE,
	SFQ_RC_EA_FUNCARG,
	SFQ_RC_EA_ILLEGALVER,
	SFQ_RC_EA_ASSERT,
	SFQ_RC_EA_SEEKSETIO,
	SFQ_RC_EA_OVERLIMIT,
	SFQ_RC_EA_RWELEMENT,
	SFQ_RC_EA_COPYVALUE,
	SFQ_RC_EA_READQFH,
	SFQ_RC_EA_WRITEQFH,
	SFQ_RC_EA_UPDSTATUS,
	SFQ_RC_EA_PATHNOTEXIST,
	SFQ_RC_EA_MKLOGDIR,
	SFQ_RC_EA_CREATENAMES,

	SFQ_RC_ES_MEMALLOC,
	SFQ_RC_ES_FILEOPEN,
	SFQ_RC_ES_FILEIO,
	SFQ_RC_ES_UNLINK,
	SFQ_RC_ES_SEMOPEN,
	SFQ_RC_ES_SEMIO,
	SFQ_RC_ES_STRDUP,
	SFQ_RC_ES_PIPE,
	SFQ_RC_ES_DUP,
	SFQ_RC_ES_WRITE,
	SFQ_RC_ES_MKDIR,
	SFQ_RC_ES_REALPATH,

	SFQ_RC_EC_EXECFAIL	= 119,
	SFQ_RC_EC_FILENOTFOUND	= 127,
};

/* PayLoad Type: uchar */
enum
{
	SFQ_PLT_NULLTERM	= 1U,
	SFQ_PLT_BINARY		= 2U,
	SFQ_PLT_CHARARRAY	= 4U,
};

/* Queue Status: uchar */
enum
{
	SFQ_QST_STDOUT_ON	= 1U,
	SFQ_QST_STDERR_ON	= 2U,
	SFQ_QST_ACCEPT_ON	= 4U,
	SFQ_QST_EXEC_ON		= 8U,
};

#define SFQ_QST_DEFAULT		(SFQ_QST_ACCEPT_ON | SFQ_QST_EXEC_ON)

/* */
struct sfq_value
{
	ulong id;				/* 8 */
	time_t pushtime;			/* 8 */
	uuid_t uuid;				/* 16 */
	char* execpath;				/* 8 */
	char* execargs;				/* 8 */
	char* metatext;				/* 8 */
	char* soutpath;				/* 8 */
	char* serrpath;				/* 8 */
	payload_type_t payload_type;		/* 1 */
	size_t payload_size;			/* 8 */
	sfq_byte* payload;			/* 8 */
};

typedef void (*sfq_map_callback)(ulong order, const struct sfq_value* val, void* userdata);

extern int sfq_init(const char* querootdir, const char* quename,
	size_t filesize_limit, size_t payloadsize_limit, ushort max_process, questate_t questate);

extern int sfq_map(const char* querootdir, const char* quename,
	sfq_map_callback callback, void* userdata);

extern int sfq_push(const char* querootdir, const char* quename, struct sfq_value* val);
extern int sfq_pop(const char* querootdir, const char* quename, struct sfq_value* val);
extern int sfq_shift(const char* querootdir, const char* quename, struct sfq_value* val);
extern int sfq_info(const char* querootdir, const char* quename);
extern int sfq_clear(const char* querootdir, const char* quename);
extern int sfq_alloc_print_value(const struct sfq_value* bin, struct sfq_value* str);
extern void sfq_free_value(struct sfq_value* p);

/* short-cut */
extern int sfq_push_text(const char* querootdir, const char* quename,
	const char* execpath, const char* execargs, const char* metatext,
	const char* soutpath, const char* serrpath, uuid_t uuid,
	const char* textdata);

extern int sfq_push_binary(const char* querootdir, const char* quename,
	const char* execpath, const char* execargs, const char* metatext,
	const char* soutpath, const char* serrpath, uuid_t uuid,
	const sfq_byte* payload, size_t payload_size);

extern int sfq_get_questate(const char* querootdir, const char* quename, questate_t* questate_ptr);
extern int sfq_set_questate(const char* querootdir, const char* quename, questate_t questate);
extern size_t sfq_payload_len(const struct sfq_value* val);

/* stack allocate and string copy */
#ifdef __GNUC__
	#define sfq_stradup(org) ({ \
			char* dst = NULL; \
			if (org) { \
				dst = alloca(strlen( (org) ) + 1); \
				if (dst) { \
					strcpy(dst, org); \
				} \
			} \
			dst; \
		})
#else
		#define sfq_stradup(org) \
			sfq_safe_strcpy( alloca( strlen( (org) ) + 1 ), (org) )

		extern char* sfq_safe_strcpy(char* dst, const char* org);
#endif

#ifdef __cplusplus
}
#endif

#endif

