#include "sfqc-lib.h"

int main(int argc, char** argv)
{
	int irc = 0;

	char* message = NULL;
	int jumppos = 0;
	questate_t questate = SFQ_QST_DEFAULT;

	ushort procs_num = 0;

/* */
	struct sfqc_init_option opt;

SFQC_MAIN_INITIALIZE

	bzero(&opt, sizeof(opt));

/* */
	irc = sfqc_get_init_option(argc, argv, "D:N:S:L:B:oe", 0, &opt);
	if (irc != 0)
	{
		message = "get_init_option: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (opt.soutpath)
	{
		if (strcmp(opt.soutpath, "-") != 0)
		{
			message = "'-o' is only allowed '-'";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}

		questate |= SFQ_QST_STDOUT_ON;
	}

	if (opt.serrpath)
	{
		if (strcmp(opt.serrpath, "-") != 0)
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
	procs_num = (opt.boota_proc_num > SFQC_RESERVE_SLOT_MIN)
		? opt.boota_proc_num : SFQC_RESERVE_SLOT_MIN;

	irc = sfq_init(opt.querootdir, opt.quename, opt.filesize_limit,
		opt.payloadsize_limit, procs_num, opt.boota_proc_num, questate);

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

	sfqc_free_init_option(&opt);

SFQC_MAIN_FINALIZE

	return irc;
}

