#include "sfqc-lib.h"

int main(int argc, char** argv)
{
	int irc = 0;
	const char* message = NULL;
	int jumppos = 0;

	uint printmethod = 0;

/* */
	struct sfqc_program_args pgargs;
	struct sfq_value val;

#ifdef FROM_XINETD
	struct sfqc_xinetd_data xd;
#endif

SFQC_MAIN_ENTER

	bzero(&pgargs, sizeof(pgargs));
	bzero(&val, sizeof(val));

#ifdef FROM_XINETD
	bzero(&xd, sizeof(xd));
#endif

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:p:q", SFQ_false, &pgargs);
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

#ifdef FROM_XINETD
	xd.pgargs = pgargs;

	irc = sfqc_xinetd_readdata(&xd);

	if (irc != 0)
	{
		message = "can't read stdin";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	pgargs = xd.pgargs;
#endif

	irc = sfq_pop(pgargs.querootdir, pgargs.quename, &val);
	if (irc != 0)
	{
		switch (irc)
		{
			case SFQ_RC_W_NOELEMENT:
			{
				message = "element does not exist in the queue";
				break;
			}
			case SFQ_RC_W_TAKEOUT_STOPPED:
			{
				message = "queue is stopped retrieval";
				break;
			}
			default:
			{
				message = "sfq_pop() fault";
				break;
			}
		}

		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	sfqc_take_success(printmethod, &val);

EXIT_LABEL:

	if (message)
	{
		sfqc_xinetd_fault(printmethod, irc, message, pgargs.quiet, __FILE__, jumppos);
	}

	sfq_free_value(&val);
	sfqc_free_program_args(&pgargs);

SFQC_MAIN_LEAVE

	return irc;
}

