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

typedef ushort questate_t;
typedef sfq_uchar payload_type_t;

enum
{
	SFQ_RC_SUCCESS			= 0,

	SFQ_RC_W_NOELEMENT		= 11,
	SFQ_RC_W_NOSPACE,
	SFQ_RC_W_ACCEPT_STOPPED,
	SFQ_RC_W_TAKEOUT_STOPPED,
	SFQ_RC_W_NOCHANGE_STATE,

	SFQ_RC_FATAL_MIN		= 21,
	SFQ_RC_UNKNOWN,
	SFQ_RC_EA_FSIZESMALL,
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
	SFQ_RC_EA_PWDNAME2ID,
	SFQ_RC_EA_NOTPERMIT,
	SFQ_RC_EA_CONCAT_N,
	SFQ_RC_EA_NOTPAYLOADTYPE,
	SFQ_RC_EA_NOTPAYLOADSIZE,
	SFQ_RC_EA_NOTPAYLOAD,
	SFQ_RC_EA_SEMEXISTS,
	SFQ_RC_EA_LOCKSEMAPHORE,
	SFQ_RC_EA_REGSEMAPHORE,

	SFQ_RC_SYSERR_MIN		= 71,
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
	SFQ_RC_ES_GETCWD,
	SFQ_RC_ES_CHMOD,
	SFQ_RC_ES_CHOWN,
	SFQ_RC_ES_SIGNAL,
	SFQ_RC_ES_CLOCKGET,

	SFQ_RC_DEV_SEMLOCK,
	SFQ_RC_DEV_SEMUNLOCK,

	SFQ_RC_EC_EXECFAIL		= 119,
	SFQ_RC_EC_FILENOTFOUND		= 127,
};

/* PayLoad Type: uchar */
enum
{
	SFQ_PLT_NULLTERM	= 1U,
	SFQ_PLT_BINARY		= 2U,
	SFQ_PLT_CHARARRAY	= 4U,
};

/* Queue Status: ushort */
enum
{
	SFQ_QST_STDOUT_ON		= 1U,
	SFQ_QST_STDERR_ON		= 2U,
	SFQ_QST_ACCEPT_ON		= 4U,
	SFQ_QST_TAKEOUT_ON		= 8U,
	SFQ_QST_EXEC_ON			= 16U,

	SFQ_QST_DEV_SEMLOCK_ON		= 32U,
	SFQ_QST_DEV_SEMUNLOCK_ON	= 64U,
};

#define SFQ_QST_DEFAULT		(SFQ_QST_ACCEPT_ON | SFQ_QST_TAKEOUT_ON | SFQ_QST_EXEC_ON)

struct sfq_queue_init_params
{
	size_t filesize_limit;
	size_t payloadsize_limit;
	ushort procs_num;
	ushort boota_proc_num;
	questate_t questate;
	const char* queuser;
	const char* quegroup;
};

/* */
struct sfq_value
{
	ulong id;				/* 8 */
	time_t pushtime;			/* 8 */
	uuid_t uuid;				/* 16 */
	const char* execpath;			/* 8 */
	const char* execargs;			/* 8 */
	const char* metatext;			/* 8 */
	const char* soutpath;			/* 8 */
	const char* serrpath;			/* 8 */
	payload_type_t payload_type;		/* 1 */
	size_t payload_size;			/* 8 */
	const sfq_byte* payload;		/* 8 */
};

typedef void (*sfq_map_callback)(ulong order, const struct sfq_value* val, void* userdata);

extern int sfq_init(const char* querootdir, const char* quename,
	const struct sfq_queue_init_params* qip);

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

extern char* sfq_alloc_concat_n(int n, ...);

/* stack allocate and string copy */
#ifdef __GNUC__
	#define sfq_stradup(org) \
		({ \
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

