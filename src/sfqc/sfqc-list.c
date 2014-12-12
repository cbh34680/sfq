#include "sfqc-lib.h"

#define _T_	"\t"
#define LF	"\n"

void print_element(ulong order, const struct sfq_value* val, void* userdata)
{
	int irc = 0;
	struct tm tmp;

	char dt[32] = "";
	char uuid_s[37] = "";

	struct sfq_value pval;

	bzero(&pval, sizeof(pval));

	localtime_r(&val->pushtime, &tmp);
	strftime(dt, sizeof(dt), "%Y-%m-%d %H:%M:%S", &tmp);

	uuid_unparse_upper(val->uuid, uuid_s);

	irc = sfq_alloc_print_value(val, &pval);
	if (irc == SFQ_RC_SUCCESS)
	{
		printf("%lu" _T_ "%zu" _T_ "%s" _T_ "%s" _T_ "%s" _T_       "%s" _T_       "%s" _T_       "%s" _T_       "%s" _T_       "%zu" _T_          "%s" LF,
		       order,    pval.id,  dt,      uuid_s,  pval.execpath, pval.execargs, pval.metatext, pval.soutpath, pval.serrpath, val->payload_size, (char*)pval.payload);

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
	struct sfqc_program_args pgargs;

/* */
SFQC_MAIN_INITIALIZE

	bzero(&pgargs, sizeof(pgargs));

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:", false, &pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = sfq_map(pgargs.querootdir, pgargs.quename, print_element, &cnt);
	if (irc != SFQ_RC_SUCCESS)
	{
		if (irc != SFQ_RC_W_NOELEMENT)
		{
			message = "sfq_map";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}
	}

	if (cnt == 0)
	{
		fprintf(stderr, "element does not exist in the queue\n");
	}
	else if (cnt == 1)
	{
		fprintf(stderr, "\nthere is one element in the queue\n");
	}
	else
	{
		fprintf(stderr, "\nthere are %zu elements in the queue\n", cnt);
	}

EXIT_LABEL:

	sfqc_free_program_args(&pgargs);

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

SFQC_MAIN_FINALIZE

	return irc;
}

