#ifndef SFQ_CMD_H_INCLUDE_ONCE_
#define SFQ_CMD_H_INCLUDE_ONCE_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <assert.h>

#ifdef WIN32
	#include "win32-dummy-build.h"
#else
	#include <inttypes.h>
	#include <dirent.h>
	#include <unistd.h>
	#include <alloca.h>
#endif

#include "sfq.h"

#ifdef SFQ_DEBUG_BUILD

#define DEBUG_PRINT_BAR		"========================="

	#define SFQC_MAIN_INITIALIZE \
		{ \
			fprintf(stderr, DEBUG_PRINT_BAR " DEBUG BUILD (%s) " DEBUG_PRINT_BAR "\n\n", argv[0]); \
			setvbuf(stdout, NULL, _IONBF, 0); \
		}

	#define SFQC_MAIN_FINALIZE \
		{ \
		}

#else
	#define SFQC_MAIN_INITIALIZE
	#define SFQC_MAIN_FINALIZE
#endif


/* プログラム引数 */
struct sfqc_init_option
{
	char* querootdir;		/* D */
	char* quename;			/* N */
	size_t filesize_limit;		/* S */
	size_t payloadsize_limit;	/* L */
	ushort procs_num;		/* R */

	char* execpath;			/* x */
	char* execargs;			/* a */
	char* textdata;			/* t */
	char* metadata;			/* m */
	char* inputfile;		/* f */

	char* soutpath;			/* o */
	char* serrpath;			/* e */

	char* printmethod;		/* p */

	const char** commands;
	int command_num;
};

/* pop, shift の表示オプション */
enum
{
	SFQC_PRM_ASRAW			= 0U,	/* ----  */
	SFQC_PRM_ASJSON			= 1U,	/* json  */
	SFQC_PRM_WITH_ATTR		= 2U,	/* witha */
	SFQC_PRM_PAYLOAD_B64		= 4U,	/* b64p  */
};

#define SFQC_PRM_DEFAULT		(SFQC_PRM_ASRAW)

/* プロトタイプ定義 */
typedef int (*sfq_takeoutfunc_t)(const char* querootdir, const char* quename, struct sfq_value* val);

extern int sfqc_takeout(int argc, char** argv, sfq_takeoutfunc_t takeoutfunc);

extern int sfqc_get_init_option(int argc, char** argv, const char* optstring,
	int use_rest, struct sfqc_init_option* p);

extern void sfqc_free_init_option(struct sfqc_init_option* p);

extern sfq_byte* sfqc_readstdin(size_t* readsize);
extern sfq_byte* sfqc_readfile(const char* path, size_t* readsize);

extern int sfqc_can_push(const struct sfqc_init_option* p);

#endif

