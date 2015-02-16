#include "sfq-lib.h"

static sfq_bool read_qh(const struct sfq_eloop_params* elop, struct sfq_q_header* qh_ptr)
{
SFQ_LIB_ENTER

	struct sfq_queue_object* qo = NULL;
	sfq_bool b = SFQ_false;
	struct sfq_file_header qfh;

/* initialize */
	bzero(&qfh, sizeof(qfh));

/* open queue-file */
	qo = sfq_open_queue_ro(elop->om_querootdir, elop->om_quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "sfq_open_queue_ro");
	}

/* read file-header */
	b = sfq_readqfh(qo, &qfh, NULL);
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "sfq_readqfh");
	}

	(*qh_ptr) = qfh.qh;

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	sfq_close_queue(qo);
	qo = NULL;

	return SFQ_LIB_IS_SUCCESS();
}

static sfq_bool update_procstate(const struct sfq_eloop_params* elop,
	sfq_uchar procstate, int TO_state, struct sfq_q_header* qh_ptr)
{
SFQ_LIB_ENTER

	struct sfq_queue_object* qo = NULL;
	struct sfq_process_info* procs = NULL;
	struct sfq_process_info* proc = NULL;
	sfq_bool b = SFQ_false;
	struct sfq_file_header qfh;

/* initialize */
	bzero(&qfh, sizeof(qfh));

/* open queue-file */
	qo = sfq_open_queue_rw(elop->om_querootdir, elop->om_quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "sfq_open_queue_rw");
	}

/* read file-header */
	b = sfq_readqfh(qo, &qfh, &procs);
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "sfq_readqfh");
	}

	if (qfh.qh.sval.procs_num <= elop->slotno)
	{
		SFQ_FAIL(EA_FUNCARG, "qfh.qh.sval.procs_num <= elop->slotno");
	}

/* update process info */
	proc = &procs[elop->slotno];

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

/* write process table */
	b = sfq_writeqfh(qo, &qfh, procs, "UPS");
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "sfq_writeqfh");
	}

/* */
	if (qh_ptr)
	{
		(*qh_ptr) = qfh.qh;
	}

SFQ_LIB_CHECKPOINT

	free(procs);
	procs = NULL;

SFQ_LIB_LEAVE

	sfq_close_queue(qo);
	qo = NULL;

	return SFQ_LIB_IS_SUCCESS();
}

sfq_bool need_sleep(ushort execable_maxla)
{
	sfq_bool b = SFQ_false;

	FILE* fp_proc = fopen("/proc/loadavg", "r");
	if (fp_proc)
	{
		float la1, la5, la10;
		int pc1, pc2;
		pid_t prev_pid;
		int set_num = 0;

		set_num = fscanf(fp_proc, "%f %f %f %d/%d %d",
			&la1, &la5, &la10, &pc1, &pc2, &prev_pid);

		fclose(fp_proc);
		fp_proc = NULL;

		if (set_num == 6)
		{
elog_print("load average 1) %.2f/ 5) %.2f/ 10) %.2f", la1, la5, la10);

			float ea_mla = (float)execable_maxla / (float)100.0;

			if (la1 > ea_mla)
			{
				b = SFQ_true;
			}
		}
	}

	return b;
}

volatile sfq_bool GLOBAL_eloop_sig_catch = SFQ_false;

static void eloop_sig_handler(int signo)
{
	signal(signo, SIG_IGN);

	GLOBAL_eloop_sig_catch = SFQ_true;
}

static void foreach_element(const struct sfq_eloop_params* elop)
{
SFQ_LIB_ENTER

	sfq_bool b = SFQ_false;
	int shift_rc = SFQ_RC_SUCCESS;
	ulong loop = 0;

	struct sfq_q_header qh;

	pid_t pid = getpid();
	pid_t ppid = getppid();

	sighandler_t save_sig_handler = SIG_ERR;

/* */
	bzero(&qh, sizeof(qh));

elog_print("#");
elog_print("# ppid    = %d", ppid);
elog_print("# pid     = %d", pid);
elog_print("# slotno  = %u", elop->slotno);
elog_print("# root    = %s", elop->om_querootdir);
elog_print("# queue   = %s", elop->om_quename);
elog_print("# execlog = %s", elop->om_queexeclogdir);
elog_print("#");

elog_print("set signal handler");

	save_sig_handler = signal(SIGTERM, eloop_sig_handler);
	if (save_sig_handler == SIG_ERR)
	{
		SFQ_FAIL(ES_SIGNAL, "set eloop_sig_handler");
	}

elog_print("before update_procstate");

	/* 状態を LOOPSTART に変更 */
	b = update_procstate(elop, SFQ_PIS_LOOPSTART, 0, &qh);
	if (! b)
	{
		SFQ_FAIL(EA_UPDSTATUS, "loop start");
	}

elog_print("before loop [questate=%u]", qh.dval.questate);

	for (loop=1; (shift_rc == SFQ_RC_SUCCESS) && (qh.dval.questate & SFQ_QST_EXEC_ON); loop++)
	{
		struct sfq_value val;
		int TO_state = SFQ_TOS_NONE;

		struct timespec tspec;

		time_t bttime = 0;
		char bttime_s[32] = "";

		char uuid_s[36 + 1] = "";
		int execrc = 0;

		int elapsed_sec = -1;

/* */
		bzero(&val, sizeof(val));

		if (clock_gettime(CLOCK_REALTIME_COARSE, &tspec) == 0)
		{
			struct tm tmbuf;

			if (localtime_r(&tspec.tv_sec, &tmbuf))
			{
				char* fmt = NULL;

				strftime(bttime_s, sizeof(bttime_s), "%y-%m-%d %H:%M:%S", &tmbuf);

				fmt = sfq_concat(bttime_s, " %03d");
				if (fmt)
				{
/*
gettimeofday():tv_usec  / 1000    = milli sec
clock_gettime():tv_nsec / 1000000 = milli sec
*/
					snprintf(bttime_s, sizeof(bttime_s), fmt, (tspec.tv_nsec / 1000000));
				}
			}

			bttime = tspec.tv_sec;
		}

elog_print("loop%zu block-top [time=%zu time_s=%s] [elm num=%lu]",
	loop, bttime, bttime_s, qh.dval.elm_num);

/*
実行可能ロードアベレージの確認
*/
		if (qh.sval.execable_maxla)
		{
elog_print("loop%zu check load-average", loop);

			if (need_sleep(qh.sval.execable_maxla))
			{
				uint rest_sec = 0;

elog_print("loop%zu load average exceeds limit, sleep ...", loop);
				rest_sec = sleep(10);

				if (rest_sec != 0)
				{
/*
シグナルによる sleep() の中断
*/
elog_print("loop%zu catch signal, break", loop);
					break;
				}

				b = read_qh(elop, &qh);
				if (! b)
				{
					SFQ_FAIL(EA_QFHRW, "read_qh");
				}

elog_print("loop%zu wake, go next loop", loop);

				continue;
elog_print("loop%zu ok, go next-step", loop);
			}
		}
		else
		{
elog_print("loop%zu no check load-average", loop);
		}

elog_print("loop%zu attempt to shift", loop);

		shift_rc = sfq_shift(elop->om_querootdir, elop->om_quename, &val);
		if (shift_rc == SFQ_RC_W_NOELEMENT)
		{
			/* no more element */

elog_print("loop%zu no more element, break", loop);

			break;
		}

		if (shift_rc == SFQ_RC_SUCCESS)
		{
			uuid_unparse(val.uuid, uuid_s);

elog_print("loop%zu shift success [id=%zu pushtime=%zu uuid=%s]",
	loop, val.id, val.pushtime, uuid_s);

elog_print("loop%zu attempt to exec [id=%zu]", loop, val.id);

			execrc = sfq_execwait(elop, &val);

			if (execrc == 0)
			{
				/* execapp() exit(== 0) */
				TO_state = SFQ_TOS_SUCCESS;
elog_print("loop%zu exec success", loop);
			}
			else if (execrc == SFQ_RC_EC_EXECFAIL)
			{
				TO_state = SFQ_TOS_CANTEXEC;
elog_print("loop%zu exec fail", loop);
			}
			else if (execrc > 0)
			{
				/* execapp() exit(<> 0) */
/*
1 - 127 のユーザが使える exit-code
*/
				TO_state = SFQ_TOS_APPEXIT_NON0;
elog_print("loop%zu exec app exit non-zero [rc=%d]", loop, execrc);
			}
			else
			{
/* 不明 */
				/* can not execapp() */
				TO_state = SFQ_TOS_CANTEXEC;
elog_print("loop%zu exec fail unknown-cause [rc=%d]", loop, execrc);
			}
		}
		else
		{
/* shift 失敗 */
			/* shift error (<> not found) */
			TO_state = SFQ_TOS_FAULT;

elog_print("loop%zu shift fail [rc=%d]", loop, shift_rc);
		}

		if (bttime != 0)
		{
			if (clock_gettime(CLOCK_REALTIME_COARSE, &tspec) == 0)
			{
				elapsed_sec = tspec.tv_sec - bttime;
			}
		}

printf("%s\t%d\t%s\t%zu\t%zu\t%d\t%d\n",
	bttime_s, shift_rc, uuid_s, val.id, val.pushtime, execrc, elapsed_sec);

		sfq_free_value(&val);

/* update to_*** */
		b = update_procstate(elop, SFQ_PIS_TAKEOUT, TO_state, &qh);

		if (! b)
		{
			SFQ_FAIL(EA_UPDSTATUS, "loop increment");
		}

elog_print("loop%zu block-bottom [questate=%u] [elapsed=%d]", loop, qh.dval.questate, elapsed_sec);
	}

SFQ_LIB_CHECKPOINT

elog_print("after loop [loop times=%zu] [sig catch=%s]", loop, GLOBAL_eloop_sig_catch ? "t" : "f");

/*
ここでの update_procstate() の失敗は無視するしかないが
スロットが埋まったままになってしまうので、解除手段の検討が必要かも
*/
	update_procstate(elop, SFQ_PIS_DONE, 0, NULL);

elog_print("after update_procstate");

	if (save_sig_handler != SIG_ERR)
	{
elog_print("restore signal handler");
		signal(SIGTERM, save_sig_handler);
	}

elog_print("--");

SFQ_LIB_LEAVE
}

sfq_bool sfq_go_exec(const char* querootdir, const char* quename,
	ushort slotno, questate_t questate)
{
	pid_t pid = (pid_t)-1;

/*
子プロセスを wait() しない
*/
	signal(SIGCHLD, SIG_IGN);

	pid = fork();

	if (pid == 0) 
	{
/* child process */
		struct sfq_open_names* om = NULL;
		struct sfq_eloop_params elop;

		mode_t dir_perm = (S_ISGID | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP);
		mode_t file_perm = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

/* */
		bzero(&elop, sizeof(elop));

		elop.slotno = slotno;
		elop.dir_perm = dir_perm;
		elop.file_perm = file_perm;

/*
ヒープに残しておくとメモリリークになるので、スタックに退避
*/
		/* copy to stack */
		om = sfq_alloc_open_names(querootdir, quename);
		assert(om);

		elop.om_querootdir = sfq_stradup(om->querootdir);
		assert(elop.om_querootdir);

		elop.om_quename = sfq_stradup(om->quename);
		assert(elop.om_quename);

		elop.om_queproclogdir = sfq_stradup(om->queproclogdir);
		assert(elop.om_queproclogdir);

		elop.om_queexeclogdir = sfq_stradup(om->queexeclogdir);
		assert(elop.om_queexeclogdir);

		sfq_free_open_names(om);
		om = NULL;

/*
子プロセスを wait() する
*/
		signal(SIGCHLD, SIG_DFL);

/*
いくつかのシグナルを無視してセッションリーダーになる
*/
		signal(SIGHUP,  SIG_IGN);
		signal(SIGINT,  SIG_IGN);
		signal(SIGQUIT, SIG_IGN);

		setsid();
/*
アンマウントを邪魔しない
*/
		chdir("/");
		umask(0);


/*
ループ処理の標準出力、標準エラー出力先を切り替え
*/
		sfq_reopen_4proc(elop.om_queproclogdir, elop.slotno, questate, file_perm);

/*
ループ処理の実行
*/
		foreach_element(&elop);

		exit (EXIT_SUCCESS);
	}

	return (pid > 0) ? SFQ_true : SFQ_false;
}

