#ifndef SFMQ_INCLUDE_H_ONCE__
#define SFMQ_INCLUDE_H_ONCE__

/* ulong, ushort ... */
#include <sys/types.h>

/* time_t */
#include <time.h>

#include <string.h>

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

#define SFQ_INPUT_STDIN			"-"
#define SFQ_DEFAULT_LOG			"@"

#define SFQ_true			(1)
#define SFQ_false			(0)

typedef unsigned char sfq_uchar;
typedef unsigned char sfq_byte;
typedef unsigned char sfq_bool;

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
	SFQ_RC_EA_OPENQUEUE,
	SFQ_RC_EA_FSIZESMALL,
	SFQ_RC_EA_EXISTQUEUE,
	SFQ_RC_EA_FUNCARG,
	SFQ_RC_EA_ILLEGALVER,
	SFQ_RC_EA_ASSERT,
	SFQ_RC_EA_OVERLIMIT,
	SFQ_RC_EA_SEEKSETIO,

	SFQ_RC_EA_UNLINKELM,
	SFQ_RC_EA_DISABLEELM,
	SFQ_RC_EA_ELMRW,
	SFQ_RC_EA_QFHRW,
	SFQ_RC_EA_COPYVALUE,
	SFQ_RC_EA_UPDSTATUS,
	SFQ_RC_EA_PATHNOTEXIST,
	SFQ_RC_EA_CREATENAMES,
	SFQ_RC_EA_PWDNAME2ID,
	SFQ_RC_EA_NOTPERMIT,

	SFQ_RC_EA_CONCAT_N,
	SFQ_RC_EA_NOTPAYLOAD,
	SFQ_RC_EA_LOCKSEMAPHORE,
	SFQ_RC_EA_REGSEMAPHORE,
	SFQ_RC_EA_ISNOTDIR,
	SFQ_RC_EA_ISINPATH,
	SFQ_RC_EA_NOTMEMBER,
	SFQ_RC_EA_PROCRUNNING,
	SFQ_RC_EA_UPDPLOCSLOT,
	SFQ_RC_EA_CHANGESIZE,

	SFQ_RC_SYSERR_MIN		= 71,
	SFQ_RC_ES_MEMORY,
	SFQ_RC_ES_FILE,
	SFQ_RC_ES_SEMAPHORE,
	SFQ_RC_ES_FORK,
	SFQ_RC_ES_MKDIR,
	SFQ_RC_ES_PATH,
	SFQ_RC_ES_CHMOD,
	SFQ_RC_ES_CHOWN,
	SFQ_RC_ES_SIGNAL,

	SFQ_RC_ES_CLOCKGET,

	SFQ_RC_USR_APPERR		= 101,

	SFQ_RC_EC_EXECFAIL		= 119,
	SFQ_RC_EC_FILENOTFOUND		= 127,
};

/* PayLoad Type: uchar */
enum
{
	SFQ_PLT_NULLTERM		= 1U,
	SFQ_PLT_BINARY			= 2U,
	SFQ_PLT_CHARARRAY		= 4U,
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
#if 0
	ushort execable_maxla;
#endif
	questate_t questate;
	const char* queusrnam;
	const char* quegrpnam;
};

/* */
struct sfq_value
{
	const char* querootdir;
	const char* quename;

	ulong id;
	time_t pushtime;
	uuid_t uuid;
	const char* eworkdir;
	const char* execpath;
	const char* execargs;
	const char* metatext;
	const char* soutpath;
	const char* serrpath;
	payload_type_t payload_type;
	size_t payload_size;
	const sfq_byte* payload;

	sfq_bool disabled;
	size_t elmsize_;
};

/* */
struct sfq_map_callback_param
{
	ulong order;
	off_t elm_pos;
	const struct sfq_value* val;
	void* userdata;

	sfq_bool disabled;
};

typedef sfq_bool (*sfq_map_callback)(struct sfq_map_callback_param* param);
/*
# typedef sfq_bool (*sfq_map_callback)(ulong order, off_t elm_pos, const struct sfq_value* val, void* userdata);
*/

#define SFQ_EXPORT __attribute__ ((visibility ("default")))

SFQ_EXPORT void sfq_set_print(sfq_bool printOnOff);
SFQ_EXPORT sfq_bool sfq_get_print();
SFQ_EXPORT size_t sfq_rtrim(char* str, const char* cmask);

SFQ_EXPORT int sfq_map_ro(const char* querootdir, const char* quename,
	sfq_map_callback callback, sfq_bool reverse, void* userdata);
SFQ_EXPORT int sfq_map_rw(const char* querootdir, const char* quename,
	sfq_map_callback callback, sfq_bool reverse, void* userdata);

SFQ_EXPORT int sfq_init(const char* querootdir, const char* quename,
	const struct sfq_queue_init_params* qip);

SFQ_EXPORT int sfq_push(const char* querootdir, const char* quename, struct sfq_value* val);

SFQ_EXPORT int sfq_pop(const char* querootdir, const char* quename, struct sfq_value* val);
SFQ_EXPORT int sfq_shift(const char* querootdir, const char* quename, struct sfq_value* val);
SFQ_EXPORT int sfq_info(const char* querootdir, const char* quename, int semlock_wait_sec);
SFQ_EXPORT int sfq_clear(const char* querootdir, const char* quename);

SFQ_EXPORT int sfq_reset_procs(const char* querootdir, const char* quename);
SFQ_EXPORT int sfq_lock_proc(const char* querootdir, const char* quename, ushort slotno);
SFQ_EXPORT int sfq_unlock_proc(const char* querootdir, const char* quename, ushort slotno);

SFQ_EXPORT int sfq_alloc_print_value(const struct sfq_value* bin, struct sfq_value* str);
SFQ_EXPORT void sfq_free_value(struct sfq_value* p);

SFQ_EXPORT void sfq_execapp(const char* execpath, const char* execargs);

/* short-cut */
SFQ_EXPORT int sfq_push_text(const char* querootdir, const char* quename,
	const char* eworkdir, const char* execpath, const char* execargs,
	const char* metatext, const char* soutpath, const char* serrpath,
	uuid_t uuid,
	const char* textdata);

SFQ_EXPORT int sfq_push_binary(const char* querootdir, const char* quename,
	const char* eworkdir, const char* execpath, const char* execargs,
	const char* metatext, const char* soutpath, const char* serrpath,
	uuid_t uuid,
	const sfq_byte* payload, size_t payload_size);

SFQ_EXPORT int sfq_get_questate(const char* querootdir, const char* quename,
	questate_t* questate_ptr, int semlock_wait_sec);

SFQ_EXPORT int sfq_set_questate(const char* querootdir, const char* quename,
	questate_t questate, int semlock_wait_sec);

SFQ_EXPORT int sfq_set_header_by_name(const char* querootdir, const char* quename,
	const char* member_name, const void* data, size_t data_size, int semlock_wait_sec);

SFQ_EXPORT size_t sfq_payload_len(const struct sfq_value* val);

/*
#char* sfq_alloc_concat_n(int n, ...);
*/
SFQ_EXPORT char* sfq_alloc_concat_NT(const char* first, ...);

#define sfq_alloc_concat(...)	sfq_alloc_concat_NT(__VA_ARGS__, NULL);

/* stack allocate and string copy */

#define sfq_stradup	strdupa

#define sfq_strandup	strndupa

#define sfq_concat(...) \
	({ \
		char* dst = NULL; \
		char* tmp_ = sfq_alloc_concat_NT(__VA_ARGS__, NULL); \
		if (tmp_) { \
			dst = sfq_stradup(tmp_); \
			free(tmp_); \
			tmp_ = NULL; \
		} \
		dst; \
	})

#ifdef __cplusplus
}
#endif

#endif

