#include "sfqc-lib.h"

sfq_bool disable_element(struct sfq_map_callback_param* param)
{
	//if (param->order == 1)
	{
		param->disabled = SFQ_true;
	}

	return SFQ_true;
}

int main(int argc, char** argv)
{
	int irc = 0;
	char* message = NULL;
	int jumppos = 0;

/* */
	struct sfqc_program_args pgargs;

/* */
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

	irc = sfq_map_rw(pgargs.querootdir, pgargs.quename, disable_element, SFQ_false, NULL);

	if (irc != SFQ_RC_SUCCESS)
	{
		if (irc != SFQ_RC_W_NOELEMENT)
		{
			message = "sfq_map";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}
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

