#include "sfqc-lib.h"

#define _T_	"\t"
#define LF	"\n"

void print_element(ulong order, const struct sfq_value* val, void* userdata)
{
	struct sfq_value pval;
	int irc = 0;
	char dt[32];
	struct tm tm;

	bzero(&pval, sizeof(pval));

	localtime_r(&val->pushtime, &tm);
	strftime(dt, sizeof(dt), "%Y-%m-%d %H:%M:%S", &tm);

	irc = sfq_alloc_print_value(val, &pval);
	if (irc == SFQ_RC_SUCCESS)
	{
		printf("%lu" _T_ "%zu" _T_ "%s" _T_ "%s" _T_       "[%s]" _T_     "[%s]" _T_     "%s" _T_       "%s" _T_       "%zu" _T_          "\"%s\"" LF,
		       order,    val->id,  dt,      pval.execpath, pval.execargs, pval.metadata, pval.soutpath, pval.serrpath, val->payload_size, (char*)pval.payload);

		sfq_free_value(&pval);
	}

	(*((ulong*)userdata))++;
}

int main(int argc, char** argv)
{
	int irc = 0;
	char* message = NULL;
	int jumppos = 0;
	ulong cnt = 0;

/* */
	struct sfqc_init_option opt;

/* */
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

	irc = sfq_map(opt.querootdir, opt.quename, print_element, &cnt);
	if (irc != SFQ_RC_SUCCESS)
	{
		if (irc != SFQ_RC_NO_ELEMENT)
		{
			message = "sfq_map";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}
	}

	if (cnt == 0)
	{
		fprintf(stderr, "\nelement does not exist in the queue\n");
	}
	else
	{
		fprintf(stderr, "\nthere are %zu elements in the queue\n", cnt);
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

