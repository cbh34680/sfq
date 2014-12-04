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

SFQ_MAIN_INITIALIZE

	atexit(release_heap);

	mem = NULL;
	bzero(&opt, sizeof(opt));

/* */
	irc = sfqc_get_init_option(argc, argv, "D:N:o:e:f:x:a:m:", 0, &opt);
	if (irc != 0)
	{
		message = "get_init_option: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = sfqc_can_push(&opt);
	if (irc != 0)
	{
		message = "sfqc_can_push";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (opt.inputfile)
	{
		if (strcmp(opt.inputfile, "-") == 0)
		{
/* read from stdin */
			mem = sfqc_readstdin(&memsize);
			if (! mem)
			{
				message = "sfqc_readstdin";
				jumppos = __LINE__;
				goto EXIT_LABEL;
			}

		}
		else
		{
/* read from file */
			mem = sfqc_readfile(opt.inputfile, &memsize);
			if (! mem)
			{
				message = "sfqc_readfile";
				jumppos = __LINE__;
				goto EXIT_LABEL;
			}
		}
	}

	irc = sfq_push_bin(opt.querootdir, opt.quename,
		opt.execpath, opt.execargs, opt.metadata,
		opt.soutpath, opt.serrpath,
		NULL,
		mem, memsize);

	if (irc != SFQ_RC_SUCCESS)
	{
		switch (irc)
		{
			case SFQ_RC_NO_SPACE:
			{
				message = "no free space in the queue";
				break;
			}
			case SFQ_RC_ACCEPT_STOPPED:
			{
				message = "queue has stop accepting";
				break;
			}
			default:
			{
				message = "sfq_push_str";
				break;
			}
		}

		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

EXIT_LABEL:

	free(mem);
	mem = NULL;

	sfqc_free_init_option(&opt);

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

SFQ_MAIN_FINALIZE

	return irc;
}

