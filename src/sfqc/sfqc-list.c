#include "sfqc-lib.h"

#define _T_	"\t"

struct prtelm_data
{
	ulong count;
	sfq_uchar loop_limit;
	sfq_bool verbose;
};

/*
# sfq_bool print_element(ulong order, off_t elm_pos, const struct sfq_value* val, void* userdata)
*/
sfq_bool print_element(struct sfq_map_callback_param* param)
{
	int irc = 0;
	struct tm tmp;

	char pushtime[32] = "";
	char uuid_s[37] = "";

	struct prtelm_data* ped = (struct prtelm_data*)param->userdata;
	struct sfq_value pval;

/* */
	bzero(&pval, sizeof(pval));

	localtime_r(&param->val->pushtime, &tmp);
	strftime(pushtime, sizeof(pushtime), "%Y-%m-%d %H:%M:%S", &tmp);

	uuid_unparse(param->val->uuid, uuid_s);

/* */
	irc = sfq_alloc_print_value(param->val, &pval);
	if (irc == SFQ_RC_SUCCESS)
	{
		if (ped->verbose)
		{
			printf("************************ %lu. element ************************\n",
				param->order);

			printf("%-13s: %s\n",  "querootdir",	pval.querootdir);
			printf("%-13s: %s\n",  "quename",	pval.quename);
			printf("%-13s: %zu\n", "offset",	param->elm_pos);
			printf("%-13s: %zu\n", "id",		pval.id);
			printf("%-13s: %s\n",  "pushtime",	pushtime);
			printf("%-13s: %s\n",  "uuid",		uuid_s);
			printf("%-13s: %s\n",  "eworkdir",	pval.eworkdir);
			printf("%-13s: %s\n",  "execpath",	pval.execpath);
			printf("%-13s: %s\n",  "execargs",	pval.execargs);
			printf("%-13s: %s\n",  "metatext",	pval.metatext);
			printf("%-13s: %s\n",  "soutpath",	pval.soutpath);
			printf("%-13s: %s\n",  "serrpath",	pval.serrpath);
			printf("%-13s: %u\n",  "payload-type",	pval.payload_type);
			printf("%-13s: %zu\n", "payload-size",	pval.payload_size);
			printf("%-13s: %d\n",  "disabled",	pval.disabled);
			printf("%-13s: %zu\n", "elmsize_",	pval.elmsize_);
			printf("%-13s: %zu\n", "* lastaddr *",	param->elm_pos + pval.elmsize_);
			printf("%-13s: %s\n",  "payload",	(char*)pval.payload);
		}
		else
		{
			printf
			(
				"%s" _T_         "%s" _T_
				"%lu" _T_     "%zu" _T_        "%zu" _T_ "%s" _T_  "%s" _T_
				"%s" _T_       "%s" _T_       "%s" _T_
				"%s" _T_       "%s" _T_       "%s" _T_
				"%u" _T_           "%zu" _T_          "%d" _T_       "%zu" _T_
				"%s"
				"\n",
				pval.querootdir, pval.quename,
				param->order, param->elm_pos,  pval.id,  pushtime, uuid_s,
				pval.eworkdir, pval.execpath, pval.execargs,
				pval.metatext, pval.soutpath, pval.serrpath,
				pval.payload_type, pval.payload_size, pval.disabled, pval.elmsize_,
				(char*)pval.payload
			);
		}
	}
	sfq_free_value(&pval);

	ped->count++;

	if (ped->loop_limit)
	{
		return ((param->order + 1) < ped->loop_limit);
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
	irc = sfqc_parse_program_args(argc, argv, "D:N:v:qr123456789", SFQ_false, &pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	sfq_set_print(pgargs.quiet ? SFQ_false : SFQ_true);

	if (pgargs.textdata)
	{
		if (strcasecmp(pgargs.textdata, "v") == 0)
		{
			ped.verbose = SFQ_true;
		}
	}

	ped.loop_limit = pgargs.num1char;

	irc = sfq_map_ro(pgargs.querootdir, pgargs.quename, print_element, pgargs.reverse, &ped);

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
		/* プログラム引数) -1 ... -9 */

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

