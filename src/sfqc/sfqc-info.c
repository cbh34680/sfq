#include "sfqc-lib.h"

int main(int argc, char** argv)
{
	int irc = 0;

	const char* message = NULL;
	int jumppos = -1;

/* */
	struct sfqc_program_args pgargs;

SFQC_MAIN_ENTER

	bzero(&pgargs, sizeof(pgargs));

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:q", SFQ_false, &pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	sfq_set_print(pgargs.quiet ? SFQ_false : SFQ_true);

	irc = sfq_info(pgargs.querootdir, pgargs.quename, SFQC_LOCK_WAIT_SEC);
	if (irc != SFQ_RC_SUCCESS)
	{
		message = "sfq_info";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

EXIT_LABEL:

	sfqc_free_program_args(&pgargs);

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

SFQC_MAIN_LEAVE

	return irc;
}

