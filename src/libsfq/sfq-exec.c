#include "sfq-lib.h"

static bool update_procstate(const char* om_querootdir, const char* om_quename, ushort slotno,
	sfq_uchar procstate, int TO_state, questate_t* questate_ptr);

static size_t atomic_write(int fd, char *buf, int count);

#define READ	(0)
#define WRITE	(1)

static void execapp(const char* execpath, char* execargs)
{
SFQ_LIB_INITIALIZE

	int argc = 0;
	char** argv = NULL;
	size_t argv_size = 0;

	int valnum = 0;

	if (execargs)
	{
/*
(カンマ区切りの) 引数の数を数える
*/
		char* pos = NULL;

		valnum = 1;

		pos = execargs;
		while (*pos)
		{
			if ((*pos) == ',')
			{
				valnum++;
			}

			pos++;
		}
	}

	argc = valnum + 2;
	argv_size = (sizeof(char*) * argc);

	argv = alloca(argv_size);
	if (! argv)
	{
		SFQ_FAIL(ES_MEMALLOC, "argv");
	}

	bzero(argv, argv_size);

/* set argv */
	argv[0] = (char*)execpath;

	if (execargs)
	{
		if (valnum == 1)
		{
			argv[1] = execargs;
		}
		else
		{
			char* pos = NULL;
			char* saveptr = NULL;
			char* token = NULL;
			int i = 0;

			for (i=1, pos=execargs; ; i++, pos=NULL)
			{
				token = strtok_r(pos, ",", &saveptr);
				if (token == NULL)
				{
					break;
				}

				if ((i + 1) >= argc)
				{
					break;
				}

				argv[i] = token;
/*
				argv[i] = sfq_stradup(token);
				if (! argv[i])
				{
					SFQ_FAIL(ES_MEMALLOC, "argv_i");
				}
*/
			}
		}
	}

/* execvp() が成功すれば処理は戻らない */

	execvp(argv[0], argv);

SFQ_LIB_CHECKPOINT

SFQ_LIB_FINALIZE
}

static int child_write_dup_exec_exit(const char* om_quename, ushort slotno, struct sfq_value* val)
{
SFQ_LIB_INITIALIZE

	char* execpath = "/bin/sh";
	char* execargs = NULL;
	int* pipefd = NULL;

	char env_ulong[32] = "";
	char uuid_s[36 + 1] = "";

/* */
#ifdef SFQ_DEBUG_BUILD
	assert(om_quename);
	assert(val);
#endif

/*
ヒープに入れたままだと exec() されたときにメモリリーク扱いになるので
payload 以外はスタックにコピー

payload は pipe 経由で送信
*/
	if (val->execpath)
	{
		execpath = sfq_stradup(val->execpath);
		if (! execpath)
		{
			SFQ_FAIL(ES_MEMALLOC, "execpath");
		}
	}

	if (val->execargs)
	{
		execargs = sfq_stradup(val->execargs);
		if (! execargs)
		{
			SFQ_FAIL(ES_MEMALLOC, "execargs");
		}
	}

	if (val->payload && val->payload_size)
	{
		int irc = 0;
		size_t wn = 0;

		/* alloc fd x 2 (READ, WRITE) */
		pipefd = alloca(sizeof(int) * 2);
		if (! pipefd)
		{
			SFQ_FAIL(ES_MEMALLOC, "pipefd");
		}

		pipefd[0] = pipefd[1] = 0;
		irc = pipe(pipefd);

		if (irc == -1)
		{
			SFQ_FAIL(ES_PIPE, "pipefd");
		}

		wn = atomic_write(pipefd[WRITE], (char*)val->payload, val->payload_size);

		if (wn != val->payload_size)
		{
			SFQ_FAIL(ES_WRITE, "atomic_write pipefd[WRITE]");
		}

		close(pipefd[WRITE]);

		irc = dup2(pipefd[READ], STDIN_FILENO);

		if (irc == -1)
		{
			SFQ_FAIL(ES_DUP, "pipefd[READ] -> STDIN_FILENO");
		}
	}

	/* queue */
	setenv("SFQ_QUENAME", om_quename, 0);

	/* id */
	snprintf(env_ulong, sizeof(env_ulong), "%zu", val->id);
	setenv("SFQ_ID", env_ulong, 0);

	/* pushtime */
	snprintf(env_ulong, sizeof(env_ulong), "%zu", val->pushtime);
	setenv("SFQ_PUSHTIME", env_ulong, 0);

	/* uuid */
	uuid_unparse(val->uuid, uuid_s);
	setenv("SFQ_UUID", uuid_s, 0);

	/* metadata */
	if (val->metadata)
	{
		setenv("SFQ_META", val->metadata, 0);
	}

	sfq_free_value(val);

/* */
	execapp(execpath, execargs);

/*
exec() が成功すればここには来ない
*/
	SFQ_FAIL(EC_EXECFAIL, "execapp");

SFQ_LIB_CHECKPOINT

	if (pipefd)
	{
		close(pipefd[READ]);
		close(pipefd[WRITE]);
	}

SFQ_LIB_FINALIZE

	return SFQ_LIB_RC();
}

static void reopen_4exec(const char* logdir, const struct sfq_value* val)
{
	time_t now = time(NULL);

	const char* soutpath = val->soutpath;
	const char* serrpath = val->serrpath;

	if (soutpath && serrpath)
	{
		if (strcmp(soutpath, serrpath) == 0)
		{
/*
標準出力先と標準エラー出力先が同じファイル名
*/
fprintf(stderr, "\tsoutpath == serrpath [%s], redirect stderr to /dev/null\n", soutpath);

			serrpath = NULL;
		}
	}

/* stdout */
	sfq_output_reopen_4exec(stdout, &now,
		soutpath, logdir, val->uuid, val->id, "out", "SFQ_SOUTPATH");

/* stderr */
	sfq_output_reopen_4exec(stderr, &now,
		serrpath, logdir, val->uuid, val->id, "err", "SFQ_SERRPATH");

/* stdio */
	freopen("/dev/null", "rb", stdin);
}

static int pipe_fork_write_dup_exec_wait(const char* om_querootdir, const char* om_quename,
	ushort slotno, const char* om_queexeclogdir, struct sfq_value* val)
{
	pid_t pid = -1;

/*
親プロセスで SIGCHLD に対して SIG_IGN を設定ているので元に戻さない
wait() 後にステータスが取得できない
*/
	signal(SIGCHLD, SIG_DFL);

	pid = fork();
	if (pid < 0)
	{
/* fault */
	}
	else
	{

		if (pid == 0)
		{
/* child */
			reopen_4exec(om_queexeclogdir, val);
			child_write_dup_exec_exit(om_quename, slotno, val);

			sfq_free_value(val);

/*
exec() が成功すればここには来ない
*/
			exit (SFQ_RC_EC_EXECFAIL);
		}
		else
		{
/* parent */
			int irc = 0;
			int status = 0;

fprintf(stderr, "\tbefore wait child-process [pid=%d]\n", pid);

			irc = waitpid(pid, &status, 0);

			if (irc != -1)
			{
				if (WIFEXITED(status))
				{
					int exit_code = WEXITSTATUS(status);

fprintf(stderr, "\tafter wait child-process [pid=%d exit=%d]\n", pid, exit_code);

					return exit_code;
				}
			}
		}
	}

	return -1;
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
		if (shift_rc == SFQ_RC_NO_ELEMENT)
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

			irc = pipe_fork_write_dup_exec_wait(om_querootdir, om_quename,
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

fprintf(stderr, "after loop [times=%zu]\n", loop);
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

/*
 * http://baohandroid.firebird.jp/createanduse/codequality%E3%83%A1%E3%83%A2%E3%80%80write%E3%82%B7%E3%82%B9%E3%83%86%E3%83%A0%E3%82%B3%E3%83%BC%E3%83%AB%E3%81%AE%E3%82%B7%E3%82%B0%E3%83%8A%E3%83%AB%E4%B8%AD%E6%96%AD%E3%81%AB%E3%82%88%E3%82%8B/
 *
 */
static size_t atomic_write(int fd, char *buf, int count)
{
	ssize_t got, need = count;
     
	while((got = write(fd, buf, need)) > 0 && (need -= got) > 0)
	{
		buf += got;
	}

	return (got < 0 ? got : count - need);
}

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

