#include "sfqc-lib.h"

int main(int argc, char** argv)
{
	int irc = 0;
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
		message = (irc == SFQ_RC_NO_ELEMENT) ? "element does not exist" : "sfq_pop";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

#ifdef SFQ_DEBUG_BUILD
	irc = sfq_alloc_print_value(&val, &pval);
	if (irc != SFQ_RC_SUCCESS)
	{
		message = "sfq_alloc_print_value";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	printf("%s\n%s\n%s\n%s\n", pval.execpath, pval.execargs, pval.metadata, pval.payload);
#else
	if (val.payload && val.payload_size)
	{
		int wrt = 0;

/*
		fprintf(stderr, "type=%u size=%zu\n", val.payload_type, val.payload_size);
*/
		if (val.payload_type & SFQ_PLT_CHARARRAY)
		{
			if (val.payload_type & SFQ_PLT_NULLTERM)
			{
				puts((char*)val.payload);
				wrt = 1;
			}
		}

		if (! wrt)
		{
			fwrite(val.payload, val.payload_size, 1, stdout);
		}
	}
#endif

EXIT_LABEL:

	sfq_free_value(&val);
	sfq_free_value(&pval);
	sfqc_free_init_option(&opt);

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

SFQ_MAIN_FINALIZE

	return irc;
}

