#include "sfqc-lib.h"

#define _T_	"\t"
#define LF	"\n"

struct prtelm_data
{
	ulong count;
	sfq_uchar loop_limit;
};

sfq_bool print_element(ulong order, off_t elm_pos, const struct sfq_value* val, void* userdata)
{
	int irc = 0;
	struct tm tmp;

	char dt[32] = "";
	char uuid_s[37] = "";

	struct prtelm_data* ped = (struct prtelm_data*)userdata;
	struct sfq_value pval;

/* */
	bzero(&pval, sizeof(pval));

	localtime_r(&val->pushtime, &tmp);
	strftime(dt, sizeof(dt), "%Y-%m-%d %H:%M:%S", &tmp);

	uuid_unparse_upper(val->uuid, uuid_s);

/* */
	irc = sfq_alloc_print_value(val, &pval);
	if (irc == SFQ_RC_SUCCESS)
	{
		printf("%lu" _T_ "%zu" _T_ "%zu" _T_ "%s" _T_ "%s" _T_
				"%s" _T_       "%s" _T_       "%s" _T_       "%s" _T_       "%s" _T_
				"%u" _T_           "%zu" _T_          "%s"                LF,

		       order,    elm_pos,  pval.id,  dt,      uuid_s,
				pval.execpath, pval.execargs, pval.metatext, pval.soutpath, pval.serrpath,
				val->payload_type, val->payload_size, (char*)pval.payload);

		sfq_free_value(&pval);
	}

	ped->count++;

	if (ped->loop_limit)
	{
		return ((order + 1) < ped->loop_limit);
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
	struct prtelm_data ped;

/* */
SFQC_MAIN_ENTER

	bzero(&pgargs, sizeof(pgargs));
	bzero(&ped, sizeof(ped));

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:r123456789", SFQ_false, &pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	ped.loop_limit = pgargs.num1char;

	irc = sfq_map(pgargs.querootdir, pgargs.quename, print_element, pgargs.reverse, &ped);

	if (irc != SFQ_RC_SUCCESS)
	{
		if (irc != SFQ_RC_W_NOELEMENT)
		{
			message = "sfq_map";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}
	}

	if (pgargs.num1char)
	{
		fprintf(stderr, "\n");
	}
	else
	{
		if (ped.count == 0)
		{
			fprintf(stderr, "element does not exist in the queue\n");
		}
		else if (ped.count == 1)
		{
			fprintf(stderr, "\nthere is one element in the queue\n");
		}
		else
		{
			fprintf(stderr, "\nthere are %zu elements in the queue\n", ped.count);
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

