#include "sfqc-lib.h"

static struct sfqc_init_option opt;
static sfq_byte* mem = NULL;

static void release_heap()
{
	free(mem);
	mem = NULL;

	sfqc_free_init_option(&opt);
}

int main(int argc, char** argv)
{
	int irc = SFQ_RC_UNKNOWN;
	char* message = NULL;
	int jumppos = 0;
	size_t memsize = 0;

/* */

SFQC_MAIN_INITIALIZE

	atexit(release_heap);

	mem = NULL;
	bzero(&opt, sizeof(opt));

/* */
	irc = sfqc_get_init_option(argc, argv, "D:N:o:e:f:x:a:m:q", false, &opt);
	if (irc != 0)
	{
		message = "get_init_option: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (opt.inputfile)
	{
		if (strcmp(opt.inputfile, "-") == 0)
		{
/* read from stdin */
			irc = sfqc_readstdin(&mem, &memsize);
			if (irc != 0)
			{
				message = "can't read stdin";
				jumppos = __LINE__;
				goto EXIT_LABEL;
			}
		}
		else
		{
/* read from file */
			irc = sfqc_readfile(opt.inputfile, &mem, &memsize);
			if (irc != 0)
			{
				message = "can't read file";
				jumppos = __LINE__;
				goto EXIT_LABEL;
			}
		}
	}

	irc = sfq_push_binary(opt.querootdir, opt.quename,
		opt.execpath, opt.execargs, opt.metatext,
		opt.soutpath, opt.serrpath,
		NULL,
		mem, memsize);

	if (irc != SFQ_RC_SUCCESS)
	{
		switch (irc)
		{
			case SFQ_RC_W_NOSPACE:
			{
				message = "no free space in the queue";
				break;
			}
			case SFQ_RC_W_ACCEPT_STOPPED:
			{
				message = "queue has stop accepting";
				break;
			}
			default:
			{
				message = "sfq_push_binary";
				break;
			}
		}

		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

EXIT_LABEL:

	free(mem);
	mem = NULL;

	if (! opt.quiet)
	{
		if (message)
		{
			fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
		}
	}

	sfqc_free_init_option(&opt);

SFQC_MAIN_FINALIZE

	return irc;
}

