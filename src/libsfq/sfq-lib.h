#ifndef SFQ_LIB_H_INCLUDE_ONCE_
#define SFQ_LIB_H_INCLUDE_ONCE_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <sys/stat.h>        /* For mode constants */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>           /* For O_* constants */
#include <signal.h>

#ifdef WIN32
	#include "win32-dummy-build.h"
#else
	#include <stdbool.h>
	#include <inttypes.h>
	#include <dirent.h>
	#include <unistd.h>
	#include <alloca.h>
	#include <semaphore.h>
	#include <wait.h>
#endif

#include "sfq.h"

#define LIBFUNC_INITIALIZE \
	int fire_line__  = -1; \
	int fire_rc__ = SFQ_RC_UNKNOWN; \
	char* fire_reason__ = NULL;


#define LIBFUNC_COMMIT \
	fire_rc__ = SFQ_RC_SUCCESS; \
FIRE_CATCH_LABEL__:


#define LIBFUNC_IS_ROLLBACK()	(fire_rc__ != SFQ_RC_SUCCESS)
#define LIBFUNC_IS_SUCCESS()	(fire_rc__ == SFQ_RC_SUCCESS)
#define LIBFUNC_RC()		(fire_rc__)


#define LIBFUNC_FINALIZE \
	if (fire_reason__) { \
		fprintf(stderr, "%s(%d): RC=%d [%s]\n", __FILE__, fire_line__, fire_rc__, fire_reason__); \
	}


#define FIRE(fire_rc, fire_reason) \
	fire_line__ = __LINE__; \
	fire_rc__ = fire_rc; \
	fire_reason__ = fire_reason; \
	goto FIRE_CATCH_LABEL__;


#ifdef WIN32
	#define sfq_stradup(org)	NULL
#else
	/* gnu only */
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
#endif

/* */
#ifndef SFQ_DEFAULT_QUEUE_DIR
	#define SFQ_DEFAULT_QUEUE_DIR	"/var/tmp"
#endif

#ifndef SFQ_DEFAULT_QUEUE_NAME
	#define SFQ_DEFAULT_QUEUE_NAME	"noname"
#endif

#define SFQ_MAGICSTR			"sfq"

#define SFQ_QUEUE_FILENAME		"queue.dat"
#define SFQ_QUEUE_LOGDIRNAME		"logs"
#define SFQ_QUEUE_PROC_LOGDIRNAME	"proc"
#define SFQ_QUEUE_EXEC_LOGDIRNAME	"exec"

#define SFQ_ALIGN_MARGIN(e)		(((( (e) / 8 ) + (( (e) % 8) ? 1 : 0)) * 8) - (e) )

/* */
struct sfq_open_names
{
	char* quename;
	char* querootdir;

	char* quedir;
	char* quefile;
	char* quelogdir;
	char* queproclogdir;
	char* queexeclogdir;
	char* semname;
};

struct sfq_queue_object
{
	struct sfq_open_names* om;

	sem_t* semobj;
	FILE* fp;
	time_t opentime;
};

enum
{
	SFQ_PIS_DONE			= 0,
	SFQ_PIS_NONE			= 0,
	SFQ_PIS_WAITFOR,
	SFQ_PIS_LOOPSTART,
	SFQ_PIS_TAKEOUT,
};

enum
{
	SFQ_TO_NONE			= 0,
	SFQ_TO_SUCCESS,
	SFQ_TO_APPEXIT_NON0,
	SFQ_TO_CANTEXEC,
	SFQ_TO_FAULT,
};

/* --------------------------------------------------------------
 *
 * ファイルに保存される構造体 (8 byte alignment)
 */

struct sfq_process_info
{
	pid_t ppid;			/* 4 */
	pid_t pid;			/* 4 */

	sfq_uchar procstatus;		/* 1 */
	sfq_byte filler[7];		/* 7 */

	time_t updtime;			/* 8 */
	ulong start_num;		/* 8 */
	ulong loop_num;			/* 8 */

	ulong to_success;		/* 8 */
	ulong to_appexit_non0;		/* 8 */
	ulong to_cantexec;		/* 8 */
	ulong to_fault;			/* 8 */
};

/* 属性ヘッダ */
struct sfq_qh_sval
{
	off_t procseg_start_pos;	/* 8 */
	off_t elmseg_start_pos;		/* 8 */
	off_t elmseg_end_pos;		/* 8 */

	size_t filesize_limit;		/* 8 */
	size_t payloadsize_limit;	/* 8 */
	time_t createtime;		/* 8 */

	ushort max_proc_num;		/* 2 ... (P) USHRT_MAX _SC_CHILD_MAX */
	sfq_byte filler[6];		/* 6 */
};

struct sfq_qh_dval
{
	off_t elm_last_push_pos;	/* 8 */
	off_t elm_new_push_pos;		/* 8 */
	off_t elm_next_shift_pos;	/* 8 */

	ulong elm_num;			/* 8 */
	ulong elm_lastid;		/* 8 */

	time_t updatetime;		/* 8 */
	ulong update_num;		/* 8 */

	char lastoper[4];		/* 4 */
	ushort questatus;		/* 1 */
	sfq_byte filler[2];		/* 2 */
};

struct sfq_q_header
{
	struct sfq_qh_sval sval;
	struct sfq_qh_dval dval;
};

/* ファイルヘッダ */
struct sfq_file_header
{
	char magicstr[4];		/* 4 = "sfq\0" */
	ushort qfh_size;		/* 2 */
	sfq_byte filler[2];		/* 2 */

	struct sfq_q_header qh;

	struct sfq_qh_dval last_qhd1;
	struct sfq_qh_dval last_qhd2;
};

/* 要素ヘッダ */
struct sfq_e_header
{
/* file offset */
	off_t prev_elmpos;		/* 8 */
	off_t next_elmpos;		/* 8 */

/* value */
	ushort eh_size;			/* 2 */
	sfq_uchar payload_type;		/* 1 */
	sfq_uchar elmmargin_;		/* 1 */
	ushort execpath_size;		/* 2 ... (x) USHRT_MAX PATH_MAX */
	ushort metadata_size;		/* 2 ... (m) USHRT_MAX */

	uint execargs_size;		/* 4 ... (a) UINT_MAX _SC_ARG_MAX */
	ushort soutpath_size;		/* 2 ... (o) USHRT_MAX PATH_MAX */
	ushort serrpath_size;		/* 2 ... (e) USHRT_MAX PATH_MAX */

	ulong id;			/* 8 */
	time_t pushtime;		/* 8 */

	size_t payload_size;		/* 8 */

	size_t elmsize_;		/* 8 ... for debug, set by sfq_copy_val2ioeb() */
};

extern void sfq_print_sizes(void);
extern void sfq_print_qf_header(const struct sfq_file_header*);
extern void sfq_print_q_header(const struct sfq_q_header*);
extern void sfq_print_qh_dval(const struct sfq_qh_dval*);
extern void sfq_print_e_header(const struct sfq_e_header*);
extern void sfq_print_procs(const struct sfq_process_info* procs, size_t procs_num);

extern struct sfq_queue_object* sfq_create_queue(const char* querootdir, const char* quename);
extern struct sfq_queue_object* sfq_open_queue(const char* querootdir, const char* quename, const char* file_mode);
extern void sfq_close_queue(struct sfq_queue_object* qo);

extern bool sfq_seek_set_and_read(FILE* fp, off_t pos, void* mem, size_t mem_size);
extern bool sfq_seek_set_and_write(FILE* fp, off_t pos, void* mem, size_t mem_size);

extern void sfq_qh_init_pos(struct sfq_q_header*);

extern bool sfq_go_exec(const char* querootdir, const char* quename, int arridx);

/* helper */
struct sfq_ioelm_buff
{
	struct sfq_e_header eh;
	char* execpath;
	char* execargs;
	char* metadata;
	sfq_byte* payload;
};

extern bool sfq_readqfh(FILE* fp, struct sfq_file_header* qfh, struct sfq_process_info** pprocs);

extern bool sfq_copy_ioeb2val(const struct sfq_ioelm_buff* ioeb, struct sfq_value* val);
extern bool sfq_copy_val2ioeb(const struct sfq_value* val, struct sfq_ioelm_buff* ioeb);

extern bool sfq_writeelm(FILE* fp, off_t seek_pos, struct sfq_ioelm_buff* ioeb);
extern bool sfq_readelm(FILE* fp, off_t seek_pos, struct sfq_ioelm_buff* ioeb);
extern void sfq_free_ioelm_buff(struct sfq_ioelm_buff* ioeb);

extern struct sfq_open_names* sfq_alloc_open_names(const char* querootdir, const char* quename);
extern void sfq_free_open_names(struct sfq_open_names* om);

#endif

