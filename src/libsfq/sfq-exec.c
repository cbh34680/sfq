#include "sfq-lib.h"

static bool update_procstatus(const char* querootdir, const char* quename, ushort slotno,
	sfq_uchar procstatus, int to_status);
static size_t atomic_write(int fd, char *buf, int count);

#define READ	(0)
#define WRITE	(1)

static void execapp(const char* execpath, char* execargs)
{
SFQ_LIB_INITIALIZE

	int argc = 0;
	char** argv = NULL;
	size_t argv_size = 0;

	char* pos = NULL;
	int valnum = 0;

	if (execargs)
	{
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

				argv[i] = sfq_stradup(token);
				if (! argv[i])
				{
					SFQ_FAIL(ES_MEMALLOC, "argv_i");
				}
			}
		}
	}


/* execvp() が成功すれば処理は戻らない */

	execvp(argv[0], argv);

SFQ_LIB_CHECKPOINT

SFQ_LIB_FINALIZE
}

static int child_write_dup_exec_exit(const char* quename, ushort slotno, struct sfq_value* val)
{
SFQ_LIB_INITIALIZE

	char* execpath = "/bin/sh";
	char* execargs = NULL;
	int* pipefd = NULL;

	char env_ulong[32] = "";
	char uuid_s[36 + 1] = "";

/* */
#ifdef SFQ_DEBUG_BUILD
	assert(quename);
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
	setenv("SFQ_QUENAME", quename, 0);

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
/* stdio */
	freopen("/dev/null", "rb", stdin);

/* stdout */
	sfq_output_reopen_4exec(stdout, val->soutpath, logdir, val->uuid, val->id, "out", "SFQ_SOUTPATH");

/* stderr */
	sfq_output_reopen_4exec(stderr, val->serrpath, logdir, val->uuid, val->id, "err", "SFQ_SERRPATH");
}

static int pipe_fork_write_dup_exec_wait(const char* querootdir, const char* quename,
	ushort slotno, struct sfq_value* val)
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
			char* om_quename = NULL;

			struct sfq_open_names* om = NULL;
			om = sfq_alloc_open_names(querootdir, quename);
#if STDIO_REOPEN_LOGFILE
			reopen_4exec(om ? om->queexeclogdir : NULL, val);
#endif
			om_quename = sfq_stradup(om->quename);
			sfq_free_open_names(om);

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

			irc = waitpid(pid, &status, 0);

			if (irc != -1)
			{
				if (WIFEXITED(status))
				{
					int exit_code = WEXITSTATUS(status);
#ifdef SFQ_DEBUG_BUILD
					fprintf(stderr, "wait) pid=%d exit=%d\n", pid, exit_code);
#endif
					return exit_code;
				}
			}
		}
	}

	return -1;
}

static void foreach_element(const char* querootdir, const char* quename, ushort slotno)
{
SFQ_LIB_INITIALIZE

	bool b = false;
	int shift_rc = 0;
	ulong loop = 0;

printf("%d) loop start (slotno=%u ppid=%d)\n", getpid(), slotno, getppid());

	/* 状態を LOOPSTART に変更 */
	b = update_procstatus(querootdir, quename, slotno, SFQ_PIS_LOOPSTART, 0);
	if (! b)
	{
		SFQ_FAIL(EA_UPDSTATUS, "loop start");
	}

	do
	{
		struct sfq_value val;
		int to_status = SFQ_TO_NONE;

		bzero(&val, sizeof(val));

		loop++;

printf("%d) loop %zu shift\n", getpid(), loop);

		shift_rc = sfq_shift(querootdir, quename, &val);
		if (shift_rc == SFQ_RC_NO_ELEMENT)
		{
			/* no more element */

printf("%d) no more element\n", getpid());

			break;
		}

		if (shift_rc == SFQ_RC_SUCCESS)
		{
			int irc = 0;

printf("%d) shift success\n", getpid());

printf("%d) exec val.id=%zu\n", getpid(), val.id);
			irc = pipe_fork_write_dup_exec_wait(querootdir, quename, slotno, &val);
			if (irc == 0)
			{
				/* execapp() exit(== 0) */
				to_status = SFQ_TO_SUCCESS;
printf("%d) exec success\n", getpid());
			}
			else if (irc == SFQ_RC_EC_EXECFAIL)
			{
				to_status = SFQ_TO_CANTEXEC;
printf("%d) exec fail\n", getpid());
			}
			else if (irc > 0)
			{
				/* execapp() exit(<> 0) */
/*
1 - 127 のユーザが使える exit-code
*/
				to_status = SFQ_TO_APPEXIT_NON0;
printf("%d) exec app exit rc=%d\n", getpid(), irc);
			}
			else
			{
/* 不明 */
				/* can not execapp() */
				to_status = SFQ_TO_CANTEXEC;
printf("%d) exec fail/2\n", getpid());
			}
		}
		else
		{
/* shift 失敗 */
			/* shift error (<> not found) */
			to_status = SFQ_TO_FAULT;

printf("%d) shift error rc=%d\n", getpid(), shift_rc);
		}

		sfq_free_value(&val);

		update_procstatus(querootdir, quename, slotno, SFQ_PIS_TAKEOUT, to_status);
	}
	while (shift_rc == SFQ_RC_SUCCESS);

SFQ_LIB_CHECKPOINT

	update_procstatus(querootdir, quename, slotno, SFQ_PIS_DONE, 0);

printf("%d) loop end %zu\n", getpid(), loop);

SFQ_LIB_FINALIZE
}

bool sfq_go_exec(const char* querootdir, const char* quename, ushort slotno)
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

			struct sfq_open_names* om = NULL;
			om = sfq_alloc_open_names(querootdir, quename);
#if STDIO_REOPEN_LOGFILE
			sfq_reopen_4proc(om ? om->queproclogdir : NULL, slotno);
#endif
			sfq_free_open_names(om);

			foreach_element(querootdir, quename, slotno);

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

static bool update_procstatus(const char* querootdir, const char* quename,
	ushort slotno, sfq_uchar procstatus, int to_status)
{
SFQ_LIB_INITIALIZE

	struct sfq_queue_object* qo = NULL;
	struct sfq_process_info* procs = NULL;
	struct sfq_process_info* proc = NULL;

	bool b = false;
	size_t procs_size = 0;

	struct sfq_file_header qfh;

/* initialize */
	bzero(&qfh, sizeof(qfh));

/* open queue-file */
	qo = sfq_open_queue(querootdir, quename, "rb+");
	if (! qo)
	{
		SFQ_FAIL(EA_OPENFILE, "sfq_open_queue");
	}

/* read file-header */
	b = sfq_readqfh(qo->fp, &qfh, &procs);
	if (! b)
	{
		SFQ_FAIL(EA_READQFH, "sfq_readqfh");
	}

	if (qfh.qh.sval.max_proc_num <= slotno)
	{
		SFQ_FAIL(EA_FUNCARG, "qfh.qh.sval.max_proc_num <= i");
	}

	procs_size = qfh.qh.sval.max_proc_num * sizeof(struct sfq_process_info);

/* update process info */
	proc = &procs[slotno];

	proc->procstatus = procstatus;
	proc->updtime = qo->opentime;

	switch (procstatus)
	{
		case SFQ_PIS_LOOPSTART:
		{
			proc->pid = getpid();
			proc->start_num++;
			break;
		}

		case SFQ_PIS_TAKEOUT:
		{
			proc->loop_num++;

			switch (to_status)
			{
				case SFQ_TO_SUCCESS:		{ proc->to_success++;		break; }
				case SFQ_TO_APPEXIT_NON0:	{ proc->to_appexit_non0++;	break; }
				case SFQ_TO_CANTEXEC:		{ proc->to_cantexec++;		break; }
				case SFQ_TO_FAULT:		{ proc->to_fault++;		break; }
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

/* write process table */
	b = sfq_seek_set_and_write(qo->fp, qfh.qh.sval.procseg_start_pos, procs, procs_size);
	if (! b)
	{
		SFQ_FAIL(EA_SEEKSETIO, "sfq_seek_set_and_write");
	}

SFQ_LIB_CHECKPOINT

	sfq_close_queue(qo);
	qo = NULL;

	free(procs);
	procs = NULL;

SFQ_LIB_FINALIZE

	return SFQ_LIB_IS_SUCCESS();
}

