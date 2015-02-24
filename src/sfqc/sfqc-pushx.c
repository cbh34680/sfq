#include "sfqc-lib.h"

static struct sfqc_xinetd_data xd;

static void release_heap()
{
	free(xd.payload);
	xd.payload = NULL;

	sfqc_free_program_args(&xd.pgargs);
}

int main(int argc, char** argv)
{
	int irc = SFQ_RC_UNKNOWN;
	const char* message = NULL;
	int jumppos = 0;

	uuid_t uuid;
	uint printmethod = 0;

/* */

SFQC_MAIN_ENTER

	bzero(&xd, sizeof(xd));
	uuid_clear(uuid);

	atexit(release_heap);

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:w:o:e:x:a:m:p:q", SFQ_false, &xd.pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	sfq_set_print(xd.pgargs.quiet ? SFQ_false : SFQ_true);

	irc = sfqc_parse_printmethod(xd.pgargs.printmethod, &printmethod);
	if (irc != 0)
	{
		message = "sfqc_parse_printmethod";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

/* read from stdin */

	irc = sfqc_xinetd_readdata(&xd);

	if (irc != 0)
	{
		message = "can't read stdin";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (xd.payload_type & SFQ_PLT_CHARARRAY)
	{
		irc = sfq_push_text(xd.pgargs.querootdir, xd.pgargs.quename,
			xd.pgargs.eworkdir, xd.pgargs.execpath, xd.pgargs.execargs,
			xd.pgargs.metatext, xd.pgargs.soutpath, xd.pgargs.serrpath,
			uuid,
			(char*)xd.payload);
	}
	else if (xd.payload_type & SFQ_PLT_BINARY)
	{
		irc = sfq_push_binary(xd.pgargs.querootdir, xd.pgargs.quename,
			xd.pgargs.eworkdir, xd.pgargs.execpath, xd.pgargs.execargs,
			xd.pgargs.metatext, xd.pgargs.soutpath, xd.pgargs.serrpath,
			uuid,
			xd.payload, xd.payload_size);
	}
	else
	{
		irc = SFQ_RC_USR_APPERR;

		message = "unknown payload_type";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

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
				message = "sfq_push_NN() fault";
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
		sfqc_xinetd_fault(printmethod, irc, message, xd.pgargs.quiet, __FILE__, jumppos);
	}

SFQC_MAIN_LEAVE

	return irc;
}

