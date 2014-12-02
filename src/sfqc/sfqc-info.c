#include "sfqc-lib.h"

int main(int argc, char** argv)
{
	int irc = 0;

	char* message = NULL;
	int jumppos = -1;

/* */
	struct sfqc_init_option opt;

SFQ_MAIN_INITIALIZE

	bzero(&opt, sizeof(opt));

/* */
	irc = sfqc_get_init_option(argc, argv, "D:N:", &opt);
	if (irc != 0)
	{
		message = "get_init_option: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = sfq_info(opt.querootdir, opt.quename);
	if (irc != SFQ_RC_SUCCESS)
	{
		message = "sfq_info";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

EXIT_LABEL:

	sfqc_free_init_option(&opt);

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

SFQ_MAIN_FINALIZE

	return irc;
}

