#include "sfqc-lib.h"

int main(int argc, char** argv)
{
	int irc = SFQ_RC_UNKNOWN;
	char* message = NULL;
	int jumppos = 0;

/* */
	struct sfqc_init_option opt;
	struct sfq_value val;
	struct sfq_value pval;

SFQ_MAIN_INITIALIZE

	bzero(&opt, sizeof(opt));
	bzero(&val, sizeof(val));
	bzero(&pval, sizeof(pval));

/* */
	irc = sfqc_get_init_option(argc, argv, "D:N:", &opt);
	if (irc != SFQ_RC_SUCCESS)
	{
		message = "get_init_option: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = sfq_pop(opt.querootdir, opt.quename, &val);
	if (irc != SFQ_RC_SUCCESS)
	{
		message = "sfq_pop";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = sfq_alloc_print_value(&val, &pval);
	if (irc != SFQ_RC_SUCCESS)
	{
		message = "sfq_alloc_print_value";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	printf("%s\n%s\n%s\n%s\n", pval.execpath, pval.execargs, pval.metadata, pval.payload);

EXIT_LABEL:

	sfq_free_value(&pval);
	sfq_free_value(&val);
	sfqc_free_init_option(&opt);

	if (message)
	{
		fprintf(stderr, "%s:%d:%s\n", __FILE__, jumppos, message);
	}

SFQ_MAIN_FINALIZE

	return irc;
}

