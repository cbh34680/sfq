#include "sfq-lib.h"

static bool update_procstate(const char* om_querootdir, const char* om_quename,
	ushort slotno, sfq_uchar procstate, int TO_state, questate_t* questate_ptr)
{
SFQ_LIB_INITIALIZE

	struct sfq_queue_object* qo = NULL;
	struct sfq_process_info* procs = NULL;
	struct sfq_process_info* proc = NULL;

	bool b = false;
	size_t procs_size = 0;
	size_t pi_size = 0;

	struct sfq_file_header qfh;

/* initialize */
	bzero(&qfh, sizeof(qfh));

/* open queue-file */
	qo = sfq_open_queue_rw(om_querootdir, om_quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENFILE, "sfq_open_queue");
	}

/* read file-header */
	b = sfq_readqfh(qo, &qfh, &procs);
	if (! b)
	{
		SFQ_FAIL(EA_READQFH, "sfq_readqfh");
	}

	if (qfh.qh.sval.procs_num <= slotno)
	{
		SFQ_FAIL(EA_FUNCARG, "qfh.qh.sval.procs_num <= i");
	}

/* update process info */
	proc = &procs[slotno];

	proc->procstate = procstate;
	proc->updtime = qo->opentime;

	switch (procstate)
	{
		case SFQ_PIS_LOOPSTART:
		{
			proc->pid = getpid();
			proc->start_cnt++;
			break;
		}

		case SFQ_PIS_TAKEOUT:
		{
			proc->loop_cnt++;

			switch (TO_state)
			{
				case SFQ_TOS_SUCCESS:		{ proc->tos_success++;		break; }
				case SFQ_TOS_APPEXIT_NON0:	{ proc->tos_appexit_non0++;	break; }
				case SFQ_TOS_CANTEXEC:		{ proc->tos_cantexec++;		break; }
				case SFQ_TOS_FAULT:		{ proc->tos_fault++;		break; }
			}

			break;
		}

		case SFQ_PIS_DONE:
		{
			proc->ppid = 0;
			proc->pid = 0;
			break;
		}
	}

/* */
	pi_size = sizeof(struct sfq_process_info);
	procs_size = pi_size * qfh.qh.sval.procs_num;

/* write process table */
	b = sfq_seek_set_and_write(qo->fp, qfh.qh.sval.procseg_start_pos, procs, procs_size);
	if (! b)
	{
		SFQ_FAIL(EA_SEEKSETIO, "sfq_seek_set_and_write");
	}

/* */
	if (questate_ptr)
	{
		(*questate_ptr) = qfh.qh.dval.questate;
	}

SFQ_LIB_CHECKPOINT

	free(procs);
	procs = NULL;

SFQ_LIB_FINALIZE

	sfq_close_queue(qo);
	qo = NULL;

	return SFQ_LIB_IS_SUCCESS();
}

static void foreach_element(const char* om_querootdir, const char* om_quename,
	ushort slotno, const char* om_queexeclogdir)
{
SFQ_LIB_INITIALIZE

	bool b = false;
	int shift_rc = SFQ_RC_SUCCESS;
	ulong loop = 0;

	questate_t questate = 0;

	pid_t pid = getpid();
	pid_t ppid = getppid();

fprintf(stderr, "#\n");
fprintf(stderr, "# ppid    = %d\n", ppid);
fprintf(stderr, "# pid     = %d\n", pid);
fprintf(stderr, "# slotno  = %u\n", slotno);
fprintf(stderr, "# root    = %s\n", om_querootdir);
fprintf(stderr, "# queue   = %s\n", om_quename);
fprintf(stderr, "# execlog = %s\n", om_queexeclogdir);
fprintf(stderr, "#\n");

	/* 状態を LOOPSTART に変更 */
	b = update_procstate(om_querootdir, om_quename, slotno,
		SFQ_PIS_LOOPSTART, 0, &questate);

	if (! b)
	{
		SFQ_FAIL(EA_UPDSTATUS, "loop start");
	}

fprintf(stderr, "before loop [questate=%u]\n", questate);

	while ((shift_rc == SFQ_RC_SUCCESS) && (questate & SFQ_QST_EXEC_ON))
	{
		struct sfq_value val;
		int TO_state = SFQ_TOS_NONE;

		bzero(&val, sizeof(val));

		loop++;

fprintf(stderr, "loop(%zu) block-top [time=%zu]\n", loop, time(NULL));

fprintf(stderr, "loop(%zu) attempt to shift\n", loop);

		shift_rc = sfq_shift(om_querootdir, om_quename, &val);
		if (shift_rc == SFQ_RC_W_NOELEMENT)
		{
			/* no more element */

fprintf(stderr, "loop(%zu) no more element, break\n", loop);

			break;
		}

		if (shift_rc == SFQ_RC_SUCCESS)
		{
			int irc = 0;
			char uuid_s[36 + 1] = "";

			uuid_unparse(val.uuid, uuid_s);

fprintf(stderr, "loop(%zu) shift success [id=%zu pushtime=%zu uuid=%s]\n", loop, val.id, val.pushtime, uuid_s);

fprintf(stderr, "loop(%zu) attempt to exec [id=%zu]\n", loop, val.id);

			irc = sfq_execwait(om_querootdir, om_quename,
				slotno, om_queexeclogdir, &val);

			if (irc == 0)
			{
				/* execapp() exit(== 0) */
				TO_state = SFQ_TOS_SUCCESS;
fprintf(stderr, "loop(%zu) exec success\n", loop);
			}
			else if (irc == SFQ_RC_EC_EXECFAIL)
			{
				TO_state = SFQ_TOS_CANTEXEC;
fprintf(stderr, "loop(%zu) exec fail\n", loop);
			}
			else if (irc > 0)
			{
				/* execapp() exit(<> 0) */
/*
1 - 127 のユーザが使える exit-code
*/
				TO_state = SFQ_TOS_APPEXIT_NON0;
fprintf(stderr, "loop(%zu) exec app exit non-zero [rc=%d]\n", loop, irc);
			}
			else
			{
/* 不明 */
				/* can not execapp() */
				TO_state = SFQ_TOS_CANTEXEC;
fprintf(stderr, "loop(%zu) exec fail unknown-cause [rc=%d]\n", loop, irc);
			}
		}
		else
		{
/* shift 失敗 */
			/* shift error (<> not found) */
			TO_state = SFQ_TOS_FAULT;

fprintf(stderr, "loop(%zu) shift fail [rc=%d]\n", loop, shift_rc);
		}

		sfq_free_value(&val);

/* update to_*** */
		b = update_procstate(om_querootdir, om_quename, slotno,
			SFQ_PIS_TAKEOUT, TO_state, &questate);

		if (! b)
		{
			SFQ_FAIL(EA_UPDSTATUS, "loop increment");
		}

fprintf(stderr, "loop(%zu) block-bottom [time=%zu questate=%u]\n", loop, time(NULL), questate);
	}

SFQ_LIB_CHECKPOINT

/*
ここでの update_procstate() の失敗は無視するしかないが
スロットが埋まったままになってしまうので、解除手段の検討が必要かも
*/
	update_procstate(om_querootdir, om_quename, slotno, SFQ_PIS_DONE, 0, NULL);

fprintf(stderr, "after loop [loop times=%zu]\n", loop);
fprintf(stderr, "\n");

SFQ_LIB_FINALIZE
}

bool sfq_go_exec(const char* querootdir, const char* quename, ushort slotno, questate_t questate)
{
	pid_t pid = (pid_t)-1;

	/* 子プロセスを wait() しない */
	signal(SIGCHLD, SIG_IGN);

	pid = fork();
	if (pid < 0)
	{
		/* fault create new-process */
		return false;
	}
	else
	{
		if (pid == 0) 
		{
/* child process */
			char* om_quename = NULL;
			char* om_querootdir = NULL;
			char* om_queproclogdir = NULL;
			char* om_queexeclogdir = NULL;

			struct sfq_open_names* om = NULL;

			/* copy to stack */
			om = sfq_alloc_open_names(querootdir, quename);
			assert(om);

			om_querootdir = sfq_stradup(om->querootdir);
			assert(om_querootdir);

			om_quename = sfq_stradup(om->quename);
			assert(om_quename);

			om_queproclogdir = sfq_stradup(om->queproclogdir);
			assert(om_queproclogdir);

			om_queexeclogdir = sfq_stradup(om->queexeclogdir);
			assert(om_queexeclogdir);

			sfq_free_open_names(om);
			om = NULL;

/*
ループ処理の標準出力、標準エラー出力先を切り替え
*/
			sfq_reopen_4proc(om_queproclogdir, slotno, questate);

/*
ループ処理の実行
*/
			foreach_element(om_querootdir, om_quename, slotno, om_queexeclogdir);

			exit (EXIT_SUCCESS);
		}
		else
		{
/* parent process */
		}
	}

	return true;
}

