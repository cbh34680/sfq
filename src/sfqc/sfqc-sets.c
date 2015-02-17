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

static int get_off_on(const char* cms[2], questate_t* bit_ptr)
{
	struct noun_bit_set nb_map[] =
	{
		{ "stdout",	SFQ_QST_STDOUT_ON },
		{ "stderr",	SFQ_QST_STDERR_ON },
		{ "accept",	SFQ_QST_ACCEPT_ON },
		{ "takeout",	SFQ_QST_TAKEOUT_ON },
		{ "exec",	SFQ_QST_EXEC_ON },

		{ "semlock",	SFQ_QST_DEV_SEMLOCK_ON },
		{ "semunlock",	SFQ_QST_DEV_SEMUNLOCK_ON },
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

static int do_change_questate(const char* querootdir, const char* quename,
	int bit_on, questate_t modify_bit)
{
	int irc = 0;

	char* message = NULL;
	int jumppos = -1;

	questate_t questate = 0;

	irc = sfq_get_questate(querootdir, quename, &questate, SFQC_LOCK_WAIT_SEC);
	if (irc != SFQ_RC_SUCCESS)
	{
		if ((modify_bit & SFQ_QST_DEV_SEMUNLOCK_ON) && (irc == SFQ_RC_EA_OPENQUEUE))
		{
/*
セマフォの解除要求時はロックされているときなので、開けなくても OK
*/
		}
		else
		{
			message = "sfq_get_questate";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}
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

	irc = sfq_set_questate(querootdir, quename, questate, SFQC_LOCK_WAIT_SEC);
	if (irc != SFQ_RC_SUCCESS)
	{
		message = "sfq_set_questate";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	fprintf(stderr, "status changed\n");

EXIT_LABEL:

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

	return irc;
}

static int do_change_unsigned(const char* querootdir, const char* quename, const char* cms[2])
{
	int irc = 0;

	char* message = NULL;
	int jumppos = -1;

	struct unsigned_state_minmax
	{
		const char* noun;
		ulong type_max;
		const char* defval_str;
		ulong (*get_defval_func)();
		const char* member_name;
	}
	usm_map[] =
	{
		{ "pslimit",	ULONG_MAX,	NULL,	NULL,			"payloadsize_limit" },
		{ "maxla",	USHRT_MAX,	"@",	sfqc_maxla_autodetect,	"execable_maxla" },
		{ "esleep",	UCHAR_MAX,	NULL,	NULL,			"execloop_sleep" },
		{ NULL,		0,		NULL,	NULL,			NULL },
	};

	struct unsigned_state_minmax* usm_set = NULL;

/* */
	for (usm_set=usm_map; usm_set->noun; usm_set++)
	{
		ulong chk_ul = 0;

		sfq_uchar uc = 0;
		ushort us = 0;
		ulong ul = 0;

		void* addr = NULL;
		size_t addr_size = 0;

/* */
		if (strcmp(cms[0], usm_set->noun) != 0)
		{
			continue;
		}

		if (usm_set->defval_str)
		{
			if (strcmp(cms[1], usm_set->defval_str) == 0)
			{
				assert(usm_set->get_defval_func);

				chk_ul = usm_set->get_defval_func();
			}
		}

		if (! chk_ul)
		{
			char* e = NULL;

			chk_ul = strtoul(cms[1], &e, 0);
			if (*e)
			{
				message = "parse number";
				jumppos = __LINE__;
				goto EXIT_LABEL;
			}
		}

		if (chk_ul > usm_set->type_max)
		{
			message = "check max value error";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}

		switch (usm_set->type_max)
		{
			case UCHAR_MAX:
			{
				uc = (sfq_uchar)chk_ul;

				addr = &uc;
				addr_size = sizeof(uc);
				break;
			}

			case USHRT_MAX:
			{
				us = (ushort)chk_ul;

				addr = &us;
				addr_size = sizeof(us);
				break;
			}

			case ULONG_MAX:
			{
				ul = chk_ul;

				addr = &ul;
				addr_size = sizeof(ul);
				break;
			}

			default:
			{
				message = "unknown type";
				jumppos = __LINE__;
				goto EXIT_LABEL;
			}
		}

		if (addr && addr_size)
		{
			irc = sfq_set_header_by_name(querootdir, quename,
				usm_set->member_name, addr, addr_size, SFQC_LOCK_WAIT_SEC);

			if (irc != SFQ_RC_SUCCESS)
			{
				message = "sfq_set_header_by_name";
				jumppos = __LINE__;
			}

			fprintf(stderr, "config changed\n");
		}
		else
		{
			message = "unknown type or size";
			jumppos = __LINE__;
		}

		goto EXIT_LABEL;
	}

	puts("unknown parameter");

EXIT_LABEL:

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

	return irc;
}

int main(int argc, char** argv)
{
	int irc = 0;

	char* message = NULL;
	int jumppos = -1;

	questate_t modify_bit = 0;
	int bit_on = 0;

/* */
	struct sfqc_program_args pgargs;

SFQC_MAIN_ENTER

	bzero(&pgargs, sizeof(pgargs));

	atexit(release_heap);

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:", SFQ_true, &pgargs);
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

	bit_on = get_off_on(pgargs.commands, &modify_bit);
	if (bit_on == -1)
	{
		irc = do_change_unsigned(pgargs.querootdir, pgargs.quename, pgargs.commands);
		if (irc != 0)
		{
			message = "unknown command";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}
	}
	else
	{
		irc = do_change_questate(pgargs.querootdir, pgargs.quename, bit_on, modify_bit);
		if (irc != 0)
		{
			message = "specify the command (noun verb)";
			jumppos = __LINE__;
			goto EXIT_LABEL;
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

