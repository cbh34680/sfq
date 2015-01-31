#include "sfqc-lib.h"

static struct sfqc_program_args pgargs;
static sfq_byte* mem = NULL;

static void release_heap()
{
	free(mem);
	mem = NULL;

	sfqc_free_program_args(&pgargs);
}

int main(int argc, char** argv)
{
	int irc = SFQ_RC_UNKNOWN;
	char* message = NULL;
	int jumppos = 0;
	size_t memsize = 0;

	uuid_t uuid;
	uint printmethod = 0;

/* */

SFQC_MAIN_ENTER

	bzero(&pgargs, sizeof(pgargs));
	uuid_clear(uuid);

	atexit(release_heap);

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:w:o:e:f:x:a:m:p:q", SFQ_false, &pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	sfq_set_print(pgargs.quiet ? SFQ_false : SFQ_true);

	irc = sfqc_parse_printmethod(pgargs.printmethod, &printmethod);
	if (irc != 0)
	{
		message = "sfqc_parse_printmethod";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (pgargs.inputfile)
	{
		if (strcmp(pgargs.inputfile, SFQ_INPUT_STDIN) == 0)
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
			irc = sfqc_readfile(pgargs.inputfile, &mem, &memsize);
			if (irc != 0)
			{
				message = "can't read file";
				jumppos = __LINE__;
				goto EXIT_LABEL;
			}
		}
	}

	irc = sfq_push_binary(pgargs.querootdir, pgargs.quename,
		pgargs.eworkdir, pgargs.execpath, pgargs.execargs,
		pgargs.metatext, pgargs.soutpath, pgargs.serrpath,
		uuid,
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
				message = "sfq_push_binary() fault";
				break;
			}
		}

		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	sfqc_push_success(printmethod, uuid);

EXIT_LABEL:

	if (message)
	{
		sfqc_xinetd_fault(printmethod, irc, message, pgargs.quiet, __FILE__, jumppos);
	}

SFQC_MAIN_LEAVE

	return irc;
}

