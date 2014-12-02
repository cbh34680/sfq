#include "sfqc-lib.h"

int main(int argc, char** argv)
{
	int irc = 0;

	char* message = NULL;
	int jumppos = 0;

/* */
	struct sfqc_init_option opt;

SFQ_MAIN_INITIALIZE

	bzero(&opt, sizeof(opt));

/* */
	irc = sfqc_get_init_option(argc, argv, "D:N:S:L:P:", &opt);
	if (irc != 0)
	{
		message = "get_init_option: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = sfq_init(opt.querootdir, opt.quename, opt.filesize_limit, opt.payloadsize_limit, opt.max_proc_num);
	if (irc != SFQ_RC_SUCCESS)
	{
		message = "sfq_init";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

EXIT_LABEL:

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

	sfqc_free_init_option(&opt);

SFQ_MAIN_FINALIZE

	return irc;
}

