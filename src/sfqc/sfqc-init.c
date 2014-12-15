#include "sfqc-lib.h"

int main(int argc, char** argv)
{
	int irc = 0;

	char* message = NULL;
	int jumppos = 0;
	questate_t questate = SFQ_QST_DEFAULT;

	ushort procs_num = 0;

/* */
	struct sfqc_program_args pgargs;
	struct sfq_queue_init_params qip;

SFQC_MAIN_ENTER

	bzero(&pgargs, sizeof(pgargs));
	bzero(&qip, sizeof(qip));

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:S:L:B:U:G:oe", false, &pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (pgargs.soutpath)
	{
		if (strcmp(pgargs.soutpath, "-") != 0)
		{
			message = "'-o' is only allowed '-'";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}

		questate |= SFQ_QST_STDOUT_ON;
	}

	if (pgargs.serrpath)
	{
		if (strcmp(pgargs.serrpath, "-") != 0)
		{
			message = "'-e' is only allowed '-'";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}

		questate |= SFQ_QST_STDERR_ON;
	}

/*
起動可能プロセス数が 0 のときでもスロットを作成しておく

--> 今後転送キューを作成した場合にスロットが 0 だとマズい
*/
	procs_num = (pgargs.boota_proc_num > SFQC_RESERVE_SLOT_MIN)
		? pgargs.boota_proc_num : SFQC_RESERVE_SLOT_MIN;

//
	qip.filesize_limit = pgargs.filesize_limit;
	qip.payloadsize_limit = pgargs.payloadsize_limit;
	qip.procs_num = procs_num;
	qip.boota_proc_num = pgargs.boota_proc_num;
	qip.questate = questate;
	qip.queuser = pgargs.queuser;
	qip.quegroup = pgargs.quegroup;

	irc = sfq_init(pgargs.querootdir, pgargs.quename, &qip);

	if (irc != SFQ_RC_SUCCESS)
	{
		message = "sfq_init";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	fprintf(stderr, "queue has been created\n");

EXIT_LABEL:

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

	sfqc_free_program_args(&pgargs);

SFQC_MAIN_LEAVE

	return irc;
}

