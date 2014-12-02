#ifndef SFMQ_INCLUDE_H_ONCE__
#define SFMQ_INCLUDE_H_ONCE__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char sfq_uchar;
typedef unsigned char sfq_byte;

enum
{
	SFQ_RC_SUCCESS		= 0,

	SFQ_RC_UNKNOWN		= 11,

	SFQ_RC_NO_ELEMENT	= 51,
	SFQ_RC_NO_SPACE,

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

/* PayLoad Type */
enum
{
	SFQ_PLT_NULLTERM	= 1,
	SFQ_PLT_BINARY		= 2,
	SFQ_PLT_CHARARRAY	= 4,	/* Null Term String */
};

struct sfq_value
{
	ulong id;				/* 8 */
	time_t pushtime;			/* 8 */

	sfq_uchar payload_type;			/* 1 */
	sfq_byte filler[7];			/* 7 */

	char* execpath;				/* 8 */
	char* execargs;				/* 8 */
	char* metadata;				/* 8 */
	char* soutpath;				/* 8 */
	char* serrpath;				/* 8 */

	size_t payload_size;			/* 8 */
	sfq_byte* payload;			/* 8 */
};

typedef void (*sfq_map_callback)(uint64_t order, const struct sfq_value* val, void* userdata);

extern int sfq_push(const char* querootdir, const char* quename, const struct sfq_value* val);
extern int sfq_pop(const char* querootdir, const char* quename, struct sfq_value* val);
extern int sfq_shift(const char* querootdir, const char* quename, struct sfq_value* val);
extern int sfq_init(const char* querootdir, const char* quename, size_t filesize_limit, size_t payloadsize_limit, uint16_t max_process);
extern int sfq_info(const char* querootdir, const char* quename);
extern int sfq_map(const char* querootdir, const char* quename, sfq_map_callback callback, void* userdata);
extern int sfq_clear(const char* querootdir, const char* quename);

extern int sfq_alloc_print_value(const struct sfq_value* bin, struct sfq_value* str);
extern void sfq_free_value(struct sfq_value* p);

/* short-cut */
extern int sfq_push_str(const char* querootdir, const char* quename, const char* execpath, const char* execargs, const char* metadata, const char* soutpath, const char* serrpath, const char* textdata);
extern int sfq_push_bin(const char* querootdir, const char* quename, const char* execpath, const char* execargs, const char* metadata, const char* soutpath, const char* serrpath, const sfq_byte* payload, size_t payload_size);

#ifdef __cplusplus
}
#endif

#endif

