#ifndef SFQ_LIB_H_INCLUDE_ONCE_
#define SFQ_LIB_H_INCLUDE_ONCE_

/*
#pragma GCC diagnostic ignored "-Wunused-label"
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>
#include <stdarg.h>

#include <errno.h>
#include <fcntl.h>           /* For O_* constants */
#include <math.h>
#include <stddef.h>

#include <inttypes.h>
#include <dirent.h>
#include <unistd.h>
#include <alloca.h>

#include <semaphore.h>
#include <pthread.h>

#include <signal.h>
#include <wait.h>

#include <sys/stat.h>        /* For mode constants */
#include <sys/capability.h>

#include <libgen.h>

#include <pwd.h>
#include <grp.h>
#include <sys/capability.h>
#include <sys/syscall.h>     /* for SYS_gettid */

#include "sfq.h"

/* --------------------------------------------------------------
 *
 * 定数／マクロ定義
 *
 */
#ifndef SFQ_DEFAULT_QUEROOTDIR
	#define SFQ_DEFAULT_QUEROOTDIR	"/var/tmp"
#endif

#ifndef SFQ_DEFAULT_QUENAME
	#define SFQ_DEFAULT_QUENAME	"noname"
#endif

#define SFQ_MAGICSTR			"sfq"

#define SFQ_QUEUE_FILENAME		"data.sfq"
#define SFQ_QUEUE_LOGDIRNAME		"logs"
#define SFQ_QUEUE_PROC_LOGDIRNAME	"proc"
#define SFQ_QUEUE_EXEC_LOGDIRNAME	"exec"

#define SFQ_ALIGN_MARGIN(e)		(((( (e) / 8 ) + (( (e) % 8) ? 1 : 0)) * 8) - (e) )

#define SFQ_PLUSINTSTR_WIDTH(var)	( ((var) == 0) ? 1 : (((int)log10( (var) )) + 1) )

#define SFQ_DEFAULT_ELOOP_SLEEP_SEC	(10)
#define SFQ_PROCLOG_SWITCH_SIZE		(10 * 1024 * 1024)

/* --------------------------------------------------------------
 *
 * uid = 0 は root になって uid_t の初期値には使えないから
 * uid_t, gid_t はマクロを利用する
 *
 */
#define SFQ_UID_NONE		( (uid_t)-1 )
#define SFQ_GID_NONE		( (gid_t)-1 )

#define SFQ_ISSET_UID(a)	( (a) != SFQ_UID_NONE )
#define SFQ_ISSET_GID(a)	( (a) != SFQ_GID_NONE )

#define SFQ_MAX(a, b)		( ( (a) > (b) ) ? (a) : (b) )
#define SFQ_MIN(a, b)		( ( (a) > (b) ) ? (b) : (a) )

/* --------------------------------------------------------------
 *
 * SFQ_LIB_xxx() マクロ
 *
 */
#define SFQ_LIB_ENTER \
	int fire_line__  = -1; \
	int fire_rc__ = SFQ_RC_UNKNOWN; \
	int fire_errno__ = 0; \
	char fire_reason__[128] = ""; \
	sfq_trc_enter(__FILE__, __LINE__, __func__);


#define SFQ_LIB_CHECKPOINT \
	fire_rc__ = SFQ_RC_SUCCESS; \
SFQ_FAIL_CATCH_LABEL__: \
	sfq_trc_leave(__FILE__, __LINE__, __func__, fire_rc__, fire_reason__);



#define SFQ_LIB_IS_FAIL()	(fire_rc__ != SFQ_RC_SUCCESS)
#define SFQ_LIB_IS_SUCCESS()	(fire_rc__ == SFQ_RC_SUCCESS)
#define SFQ_LIB_RC()		(fire_rc__)


#define SFQ_LIB_LEAVE \
	if (fire_reason__[0]) { \
		if (sfq_get_print()) { \
			struct tm tm_tmp__; \
			time_t now__; \
			char nowstr__[32] = ""; \
			char fire_errstr__[128] = "-----"; \
			now__ = time(NULL); \
			localtime_r(&now__, &tm_tmp__); \
			strftime(nowstr__, sizeof(nowstr__), "%Y-%m-%d %H:%M:%S", &tm_tmp__); \
			if (fire_errno__) { \
				strerror_r(fire_errno__, fire_errstr__, sizeof(fire_errstr__)); \
			} \
			fprintf(stderr, "= %s =\n\t%s(%d)# %s\n\t" \
				"ppid=%d pid=%d tid=%d\n\t" \
				"reason=%d [%s]\n\t" \
				"errno=%d [%s]\n\n", \
				nowstr__, __FILE__, fire_line__, __func__, \
				getppid(), getpid(), sfq_gettid(), \
				fire_rc__, fire_reason__, \
				fire_errno__, fire_errstr__); \
		} \
	}


#define SFQ_FAIL_SILENT(fire_rc) \
	fire_line__ = __LINE__; \
	fire_rc__ = SFQ_RC_ ## fire_rc; \
	fire_errno__  = errno; \
	goto SFQ_FAIL_CATCH_LABEL__;


/*
"fmt, ##__VA_ARGS__" のようにすることで可変長引数部がゼロ個のとき、最後のカンマを除去してくれる
(GCC独自拡張)
*/
/*
#define SFQ_FAIL(fire_rc, fmt, ...) \
	fire_line__ = __LINE__; \
	fire_rc__ = SFQ_RC_ ## fire_rc; \
	fire_errno__  = errno; \
	snprintf(fire_reason__, sizeof(fire_reason__), fmt, ##__VA_ARGS__); \
	goto SFQ_FAIL_CATCH_LABEL__;
*/


#define SFQ_FAIL_V(fire_rc, fmt, ...) \
	fire_line__ = __LINE__; \
	fire_rc__ = fire_rc; \
	fire_errno__  = errno; \
	snprintf(fire_reason__, sizeof(fire_reason__), fmt, ##__VA_ARGS__); \
	goto SFQ_FAIL_CATCH_LABEL__;


#define SFQ_FAIL(fire_rc, fmt, ...) \
	SFQ_FAIL_V(SFQ_RC_ ## fire_rc, fmt, ##__VA_ARGS__);


#define elog_print(format_, ...)	fprintf(stderr, "%s(%d)\t" format_ "\n", __FILE__, __LINE__, ##__VA_ARGS__)

/* --------------------------------------------------------------
 *
 *
 */
#define SFQ_ENTP_ENTER \
\
	sfq_trc_open(); \
SFQ_LIB_ENTER \
	errno = 0;


#define SFQ_ENTP_LEAVE \
\
SFQ_LIB_LEAVE


/* --------------------------------------------------------------
 *
 *
 */
#ifdef SFQ_TRACE_FILE
	void sfq_trc_open();
	void sfq_trc_enter(const char* file, int line, const char* func);
	void sfq_trc_leave(const char* file, int line, const char* func, int rc, const char* msg);
	void sfq_trc_close();
#else
	#define sfq_trc_open()
	#define sfq_trc_enter(a, b, c)
	#define sfq_trc_leave(a, b, c, d, e)
	#define sfq_trc_close()
#endif


/* --------------------------------------------------------------
 *
 * enum 定数定義
 *
 */

/* file open mode */
enum
{
	SFQ_FOM_READ			= 1U,
	SFQ_FOM_WRITE			= 2U,
};

/* proccess info state */
enum
{
	SFQ_PIS_DONE			= 0,
	SFQ_PIS_NONE			= 0,
	SFQ_PIS_WAITFOR,
	SFQ_PIS_LOOPSTART,
	SFQ_PIS_TAKEOUT,
	SFQ_PIS_LOCK,
};

/* take out state */
enum
{
	SFQ_TOS_NONE			= 0,
	SFQ_TOS_SUCCESS,
	SFQ_TOS_APPEXIT_NON0,
	SFQ_TOS_CANTEXEC,
	SFQ_TOS_FAULT,
};

/* --------------------------------------------------------------
 *
 * ファイルに保存される構造体 (8 byte alignment)
 *
 */

/* A) 80 プロセス情報 */
struct sfq_process_info
{
	pid_t ppid;			/* 4 */
	pid_t pid;			/* 4 */

	sfq_uchar procstate;		/* 1 */
	sfq_byte filler1[7];		/* 7 */

	time_t updtime;			/* 8 */
	ulong start_cnt;		/* 8 */
	ulong loop_cnt;			/* 8 */

	ulong tos_success;		/* 8 */
	ulong tos_appexit_non0;		/* 8 */
	ulong tos_cantexec;		/* 8 */
	ulong tos_fault;		/* 8 */

	sfq_byte reserve[8];		/* 8 */
};

/* B) 80 静的属性 */
struct sfq_qh_sval
{
	off_t procseg_start_pos;	/* 8 */
	off_t elmseg_start_pos;		/* 8 */
	off_t elmseg_end_pos;		/* 8 */

	size_t filesize_limit;		/* 8 */
	size_t payloadsize_limit;	/* 8 */
	time_t createtime;		/* 8 */

	ushort procs_num;		/* 2 ... (P) USHRT_MAX _SC_CHILD_MAX */
	ushort execable_maxla;		/* 2 ... (E) USHRT_MAX */
	ushort execloop_sleep;		/* 2 ... --- UCHAR_MAX */
	sfq_byte filler1[2];		/* 2 */

	sfq_byte reserve[24];		/* 24 */
};

/* C) 80 動的属性 */
struct sfq_qh_dval
{
	off_t elm_next_pop_pos;		/* 8 */
	off_t elm_next_push_pos;	/* 8 */
	off_t elm_next_shift_pos;	/* 8 */

	ulong elm_num;			/* 8 */
	ulong elm_lastid;		/* 8 */

	time_t updatetime;		/* 8 */
	ulong update_cnt;		/* 8 */

	char lastoper[4];		/* 4 */
	questate_t questate;		/* 2 */
	sfq_byte filler1[2];		/* 2 */

	size_t elmsize_total_;		/* 8 ... for debug */

	sfq_byte reserve[8];		/* 8 */
};

/* D) 160 静的 + 動的属性 (80 + 80) */
struct sfq_q_header
{
	struct sfq_qh_sval sval;
	struct sfq_qh_dval dval;
};

/* E) 16 ファイル先頭マーク */
struct sfq_file_stamp
{
	char magicstr[4];		/* 4 = "sfq\0" */
	sfq_byte filler[2];		/* 2 */
	ushort qfh_size;		/* 2 */

	sfq_byte reserve[8];		/* 8 */
};

/* F) 336 ファイルヘッダ (16 + 160 + 80 + 80) */
struct sfq_file_header
{
	struct sfq_file_stamp qfs;	/* qfs の位置は変更 NG */

	struct sfq_q_header qh;
	struct sfq_qh_dval last_qhd1;
	struct sfq_qh_dval last_qhd2;
};

/* G) 96 要素ヘッダ */
struct sfq_e_header
{
/* file offset */
	off_t prev_elmpos;		/* 8 */
	off_t next_elmpos;		/* 8 */

/* value */
	ushort eh_size;			/* 2 */
	payload_type_t payload_type;	/* 1 */
	sfq_uchar elmmargin_;		/* 1 ... for debug, set by sfq_copy_val2ioeb() */
	sfq_bool disabled;		/* 1 */
	sfq_byte filler1[3];		/* 3 */

	ushort eworkdir_size;		/* 2 ... (x) USHRT_MAX PATH_MAX */
	ushort execpath_size;		/* 2 ... (x) USHRT_MAX PATH_MAX */
	ushort metatext_size;		/* 2 ... (m) USHRT_MAX */
	sfq_byte filler2[2];		/* 2 */

	uint execargs_size;		/* 4 ... (a) UINT_MAX _SC_ARG_MAX */
	ushort soutpath_size;		/* 2 ... (o) USHRT_MAX PATH_MAX */
	ushort serrpath_size;		/* 2 ... (e) USHRT_MAX PATH_MAX */

	ulong id;			/* 8 */
	time_t pushtime;		/* 8 */
	uuid_t uuid;			/* 16 */
	size_t payload_size;		/* 8 */

	size_t elmsize_;		/* 8 ... for debug, set by sfq_copy_val2ioeb() */

	sfq_byte reserve[8];		/* 8 */
};

/* --------------------------------------------------------------
 *
 * 関数へのパラメータ構造体
 *
 */

struct sfq_open_names
{
	const char* querootdir;
	const char* quename;

	const char* quedir;
	const char* quefile;
	const char* quelogdir;
	const char* queproclogdir;
	const char* queexeclogdir;
	const char* semname;
};

struct sfq_queue_object
{
	struct sfq_open_names* om;
	time_t opentime;
	sfq_uchar queue_openmode;
	FILE* fp;

	mode_t save_umask;
};

struct sfq_ioelm_buff
{
	struct sfq_e_header eh;
	const sfq_byte* payload;
	const char* eworkdir;
	const char* execpath;
	const char* execargs;
	const char* metatext;
	const char* soutpath;
	const char* serrpath;
};

struct sfq_eloop_params
{
	ushort slotno;

	const char* om_quename;
	const char* om_querootdir;

	const char* om_queproclogdir;
	const char* om_queexeclogdir;

	mode_t dir_perm;
	mode_t file_perm;
};

struct sfq_queue_create_params
{
	const char* quename;
	const char* querootdir;
	uid_t queusrid;
	gid_t quegrpid;
	sfq_bool chmod_GaW;
};

/* --------------------------------------------------------------
 *
 * 関数プロトタイプ
 *
 */
pid_t sfq_gettid(void);
sfq_bool sfq_caps_isset(cap_value_t cap);

sfq_bool sfq_pwdgrp_nam2id(const char* queuser, const char* quegroup,
	uid_t* usrid_ptr, gid_t* grpid_ptr);

sfq_bool sfq_pwdgrp_id2nam_alloc(uid_t uid, gid_t gid,
        const char** usrnam_ptr, const char** grpnam_ptr);

int sfq_count_char(char delim, const char* searchstr);
char** sfq_alloc_split(char delim, const char* orgstr, int* num_ptr);
void sfq_free_strarr(char** strarr);

void sfq_print_sizes(void);
void sfq_print_qo(const struct sfq_queue_object* qo);
void sfq_print_qf_header(const struct sfq_file_header*);
void sfq_print_q_header(const struct sfq_q_header*);
void sfq_print_qh_dval(const struct sfq_qh_dval*);
void sfq_print_e_header(const struct sfq_e_header*);
void sfq_print_procs(const struct sfq_process_info* procs, size_t procs_num);
void sfq_close_queue(struct sfq_queue_object* qo);
void sfq_qh_init_pos(struct sfq_q_header*);
void sfq_free_ioelm_buff(struct sfq_ioelm_buff* ioeb);
void sfq_free_open_names(struct sfq_open_names* om);
void sfq_reopen_4proc(const char* logdir, ushort slotno, questate_t questate, mode_t file_perm);

size_t sfq_get_smallest_elmsize();
void sfq_init_ioeb(struct sfq_ioelm_buff* ioeb);

sfq_bool sfq_lock_semaphore(const char* semname, int semlock_wait_sec);
void sfq_unlock_semaphore(const char* semname);

struct sfq_queue_object* sfq_open_queue_rw(const char* querootdir, const char* quename);

struct sfq_queue_object* sfq_open_queue_rw_lws(const char* querootdir, const char* quename,
	int semlock_wait_sec);

struct sfq_queue_object* sfq_open_queue_ro(const char* querootdir, const char* quename);

struct sfq_queue_object* sfq_open_queue_ro_lws(const char* querootdir, const char* quename,
	int semlock_wait_sec);

struct sfq_queue_object* sfq_open_queue_wo(const struct sfq_queue_create_params* qcp);
struct sfq_open_names* sfq_alloc_open_names(const char* querootdir, const char* quename);

int sfq_reserve_proc(struct sfq_process_info* procs, ushort procs_num);

sfq_bool sfq_writeelm(struct sfq_queue_object* qo, off_t seek_pos, const struct sfq_ioelm_buff* ioeb);
sfq_bool sfq_readelm_alloc(struct sfq_queue_object* qo, off_t seek_pos, struct sfq_ioelm_buff* ioeb);

/*
sfq_bool sfq_seek_set_and_read(FILE* fp, off_t pos, void* mem, size_t mem_size);
sfq_bool sfq_seek_set_and_write(FILE* fp, off_t pos, const void* mem, size_t mem_size);
*/

sfq_bool sfq_go_exec(const char* querootdir, const char* quename, ushort slotno, questate_t questate);
sfq_bool sfq_copy_ioeb2val(const struct sfq_ioelm_buff* ioeb, struct sfq_value* val);
sfq_bool sfq_copy_val2ioeb(const struct sfq_value* val, struct sfq_ioelm_buff* ioeb);
sfq_bool sfq_mkdir_p(const char *arg, mode_t mode);

sfq_bool sfq_readqfh(struct sfq_queue_object* qo,
	struct sfq_file_header* qfh, struct sfq_process_info** procs_ptr);

sfq_bool sfq_writeqfh(struct sfq_queue_object* qo, struct sfq_file_header* qfh,
	const struct sfq_process_info* procs, const char* lastoper);

sfq_bool sfq_disable_elm(struct sfq_queue_object* qo, off_t seek_pos, sfq_bool disabled);
sfq_bool sfq_unlink_prevelm(struct sfq_queue_object* qo, off_t seek_pos);
sfq_bool sfq_unlink_nextelm(struct sfq_queue_object* qo, off_t seek_pos);
sfq_bool sfq_link_nextelm(struct sfq_queue_object* qo, off_t seek_pos, off_t updval);

void sfq_output_reopen_4exec(FILE* fp, const time_t* now, const char* arg_wpath,
	const char* logdir, const uuid_t uuid, ulong id, const char* ext, const char* env_key,
	mode_t dir_perm, mode_t file_perm);

void sfq_write_execrc(const char* logdir, const uuid_t uuid, int rc);

int sfq_execwait(const struct sfq_eloop_params* elop, struct sfq_value* val);

#endif

