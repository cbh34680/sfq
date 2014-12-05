#include "sfqc-lib.h"

int main(int argc, char** argv)
{
	int irc = 0;
	char* message = NULL;
	int jumppos = 0;

/* */
	struct sfqc_init_option opt;

SFQC_MAIN_INITIALIZE

	bzero(&opt, sizeof(opt));

/* */
	irc = sfqc_get_init_option(argc, argv, "D:N:", 0, &opt);
	if (irc != 0)
	{
		message = "get_init_option: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = sfq_clear(opt.querootdir, opt.quename);
	if (irc != SFQ_RC_SUCCESS)
	{
		message = "sfq_clear";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

EXIT_LABEL:

	sfqc_free_init_option(&opt);

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

SFQC_MAIN_FINALIZE

	return irc;
}

