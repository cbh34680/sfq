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

	#define SFQ_MAIN_INITIALIZE \
		{ \
			fprintf(stderr, DEBUG_PRINT_BAR " DEBUG BUILD (%s) " DEBUG_PRINT_BAR "\n\n", argv[0]); \
			setvbuf(stdout, NULL, _IONBF, 0); \
		}

	#define SFQ_MAIN_FINALIZE \
		{ \
		}

#else
	#define SFQ_MAIN_INITIALIZE
	#define SFQ_MAIN_FINALIZE
#endif

/* プログラム引数 */
struct sfqc_init_option
{
	char* querootdir;		/* D */
	char* quename;			/* N */
	size_t filesize_limit;		/* S */
	size_t payloadsize_limit;	/* L */
	ushort max_proc_num;		/* P */

	char* execpath;			/* x */
	char* execargs;			/* a */
	char* textdata;			/* t */
	char* metadata;			/* m */
	char* inputfile;		/* f */

	char* soutpath;			/* o */
	char* serrpath;			/* e */
};

extern int sfqc_get_init_option(int argc, char** argv, const char* optstring, struct sfqc_init_option* p);
extern void sfqc_free_init_option(struct sfqc_init_option* p);

extern sfq_byte* sfqc_readstdin(size_t* readsize);
extern sfq_byte* sfqc_readfile(const char* path, size_t* readsize);

extern int sfqc_can_push(const struct sfqc_init_option* p);

#endif

