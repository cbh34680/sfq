#ifndef SFQ_CMD_H_INCLUDE_ONCE_
#define SFQ_CMD_H_INCLUDE_ONCE_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdarg.h>

#ifdef WIN32
	#include "win32-dummy-build.h"
#else
	#include <inttypes.h>
	#include <dirent.h>
	#include <unistd.h>
	#include <alloca.h>
	#include <regex.h>
#endif

#include "sfq.h"

#ifdef SFQ_DEBUG_BUILD

	#define DEBUG_PRINT_BAR		"========================="

	#define SFQC_MAIN_ENTER \
		{ \
			fprintf(stderr, DEBUG_PRINT_BAR " DEBUG BUILD (%s) " DEBUG_PRINT_BAR "\n\n", argv[0]); \
			setvbuf(stdout, NULL, _IONBF, 0); \
		}

	#define SFQC_MAIN_LEAVE \
		{ \
		}

#else
	#define SFQC_MAIN_ENTER
	#define SFQC_MAIN_LEAVE
#endif

#define SFQC_RESERVE_SLOT_MIN		(4)

#define SFQC_CRLF			"\r\n"
#define SFQC_LF				"\n"


/* プログラム引数 */
struct sfqc_program_args
{
	const char* querootdir;		/* D */
	const char* quename;		/* N */
	size_t filesize_limit;		/* S */
	size_t payloadsize_limit;	/* L */
	ushort boota_proc_num;		/* B */
	const char* usrnam;		/* U */
	const char* grpnam;		/* G */

	const char* eworkdir;		/* w */
	const char* execpath;		/* x */
	const char* execargs;		/* a */
	const char* textdata;		/* v */
	const char* metatext;		/* m */
	const char* inputfile;		/* f */
	const char* soutpath;		/* o */
	const char* serrpath;		/* e */
	const char* printmethod;	/* p */

	sfq_bool quiet;			/* q */
	sfq_bool reverse;		/* r */
	sfq_uchar num1char;		/* -0 to -9 */

	const char** commands;
	int command_num;
};

struct sfqc_xinetd_data
{
	struct sfqc_program_args pgargs;

	sfq_byte* payload;
	size_t payload_size;
	payload_type_t payload_type;
	size_t payload_len;
};

/* pop, shift の表示オプション */
enum
{
	SFQC_PRM_ASRAW			= 0U,	/* ---- */
	SFQC_PRM_ASJSON			= 1U,	/* json */

	SFQC_PRM_PAYLOAD_BASE64		= 2U,	/* pb64 */

	SFQC_PRM_HTTP_HEADER		= 4U,	/* http */
	SFQC_PRM_ADD_ATTRIBUTE		= 8U,	/* adda */
};

#define SFQC_PRM_DEFAULT		(SFQC_PRM_ASRAW)

/* プロトタイプ定義 */

int sfqc_parse_program_args(int argc, char** argv, const char* optstring,
	sfq_bool use_rest, struct sfqc_program_args* pgargs);

int sfqc_parse_printmethod(const char* arg, uint* printmethod_ptr);

void sfqc_free_program_args(struct sfqc_program_args* pgargs);

char** sfqc_split(char* params_str, char c_delim);
int sfqc_readstdin(sfq_byte** mem_ptr, size_t* memsize_ptr);
int sfqc_readfile(const char* path, sfq_byte** mem_ptr, size_t* memsize_ptr);

void sfqc_h_printf(sfq_bool http, const char* org_format, ...);
void sfqc_print_http_headers(sfq_bool exist, const char* content_type, size_t content_length);

void sfqc_push_success(uint printmethod, uuid_t uuid);
void sfqc_xinetd_fault(uint printmethod, int result_code, const char* message,
        sfq_bool quiet, const char* e_fn, int e_pos);

void sfqc_rtrim(char* p);
int sfqc_xinetd_readdata(struct sfqc_xinetd_data* xd);

void sfqc_take_success(uint printmethod, const struct sfq_value* val);

#endif

