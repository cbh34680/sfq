#include "sfqc-lib.h"

struct sfqc_program_args pgargs;

void release_heap()
{
	sfqc_free_program_args(&pgargs);
}

int main(int argc, char** argv)
{
	int irc = SFQ_RC_UNKNOWN;
	char* message = NULL;
	int jumppos = 0;

	sfq_byte* mem = NULL;

	atexit(release_heap);

/* */

SFQC_MAIN_ENTER

	bzero(&pgargs, sizeof(pgargs));

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:o:e:f:x:a:m:qv:", false, &pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}


	if (pgargs.inputfile)
	{
		if (strcmp(pgargs.inputfile, "-") == 0)
		{
/* read from stdin */
			irc = sfqc_readstdin(&mem, NULL);
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
			irc = sfqc_readfile(pgargs.inputfile, &mem, NULL);
			if (irc != 0)
			{
				message = "can't read file";
				jumppos = __LINE__;
				goto EXIT_LABEL;
			}
		}

		free((char*)pgargs.textdata);
		pgargs.textdata = (char*)mem;
	}

	irc = sfq_push_text(pgargs.querootdir, pgargs.quename,
		pgargs.execpath, pgargs.execargs, pgargs.metatext,
		pgargs.soutpath, pgargs.serrpath,
		NULL,
		pgargs.textdata);

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
				message = "sfq_push_text";
				break;
			}
		}

		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

EXIT_LABEL:

	free(mem);
	mem = NULL;

	if (! pgargs.quiet)
	{
		if (message)
		{
			fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
		}
	}

	sfqc_free_program_args(&pgargs);

SFQC_MAIN_LEAVE

	return irc;
}

