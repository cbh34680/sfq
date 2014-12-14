#include "sfqc-lib.h"

struct sfqc_program_args pgargs;

void release_heap()
{
	sfqc_free_program_args(&pgargs);
}

struct noun_bit_set
{
	const char* noun;
	questate_t bit;
};

int get_off_on(const char* cms[2], questate_t* bit_ptr)
{
	struct noun_bit_set nb_map[] =
	{
		{ "stdout",	SFQ_QST_STDOUT_ON },
		{ "stderr",	SFQ_QST_STDERR_ON },
		{ "accept",	SFQ_QST_ACCEPT_ON },
		{ "takeout",	SFQ_QST_TAKEOUT_ON },
		{ "exec",	SFQ_QST_EXEC_ON },

		{ "semlock",	SFQ_QST_DEV_SEMLOCK_ON },
	};

	const char* off_on[] = { "off", "on" };

	int i = 0;

	for (i=0; i<(sizeof(nb_map) / sizeof(nb_map[0])); i++)
	{
		struct noun_bit_set* nb_set = &nb_map[i];

		if (strcmp(cms[0], nb_set->noun) == 0)
		{
			int j = 0;

			for (j=0; j<(sizeof(off_on) / sizeof(off_on[0])); j++)
			{
				if (strcmp(cms[1], off_on[j]) == 0)
				{
					(*bit_ptr) = nb_set->bit;
					return j;
				}
			}
		}
	}

	return -1;
}

int main(int argc, char** argv)
{
	int irc = 0;

	char* message = NULL;
	int jumppos = -1;
	questate_t questate = 0;
	questate_t modify_bit = 0;
	int bit_on = 0;

/* */
	struct sfqc_program_args pgargs;

SFQC_MAIN_INITIALIZE

	bzero(&pgargs, sizeof(pgargs));

	atexit(release_heap);

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:", true, &pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (pgargs.command_num != 2)
	{
		message = "specify the command (noun verb)";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = sfq_get_questate(pgargs.querootdir, pgargs.quename, &questate);
	if (irc != SFQ_RC_SUCCESS)
	{
		message = "sfq_get_questate";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	bit_on = get_off_on(pgargs.commands, &modify_bit);
	if (bit_on == -1)
	{
		message = "unknown command";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (bit_on)
	{
		if (questate & modify_bit)
		{
			// no change
			message = "it is specified in current status";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}
		else
		{
			// go next
		}
	}
	else
	{
		if (questate & modify_bit)
		{
			// go next
		}
		else
		{
			// no change
			message = "it is specified in current status";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}
	}

	questate ^= modify_bit;

	irc = sfq_set_questate(pgargs.querootdir, pgargs.quename, questate);
	if (irc != SFQ_RC_SUCCESS)
	{
		message = "sfq_set_questate";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	fprintf(stderr, "status changed\n");

EXIT_LABEL:

	sfqc_free_program_args(&pgargs);

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

SFQC_MAIN_FINALIZE

	return irc;
}

