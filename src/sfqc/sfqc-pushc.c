#include "sfqc-lib.h"

struct sfqc_init_option opt;

void release_heap()
{
	sfqc_free_init_option(&opt);
}

int main(int argc, char** argv)
{
	int irc = SFQ_RC_UNKNOWN;
	char* message = NULL;
	int jumppos = 0;

	atexit(release_heap);

/* */

SFQC_MAIN_INITIALIZE

	bzero(&opt, sizeof(opt));

/* */
	irc = sfqc_get_init_option(argc, argv, "D:N:o:e:x:a:m:t:q", 0, &opt);
	if (irc != 0)
	{
		message = "get_init_option: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (opt.execpath || opt.execargs || opt.textdata)
	{
		// go next
	}
	else
	{
		message = "no input data";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = sfq_push_str(opt.querootdir, opt.quename,
		opt.execpath, opt.execargs, opt.metadata,
		opt.soutpath, opt.serrpath,
		NULL,
		opt.textdata);

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

