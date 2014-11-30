#include "sfq-lib.h"

#ifdef SFQ_DEBUG_BUILD
	#define STDIO_REOPEN_LOGFILE	(0)
#else
	#define STDIO_REOPEN_LOGFILE	(1)
#endif

static bool update_procstatus(const char* querootdir, const char* quename, int slotno,
	sfq_uchar procstatus, int to_status);
static size_t atomic_write(int fd, char *buf, int count);

#if STDIO_REOPEN_LOGFILE
static void default_reopen(const char* logdir);
#endif

#define READ	(0)
#define WRITE	(1)

static void execapp(const char* execpath, char* execargs)
{
LIBFUNC_INITIALIZE

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
		FIRE(SFQ_RC_ES_MEMALLOC, "argv");
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

				if (i >= (argc - 2))
				{
					break;
				}

				argv[i] = sfq_stradup(token);
				if (! argv[i])
				{
					FIRE(SFQ_RC_ES_MEMALLOC, "argv_i");
				}
			}
		}
	}

/* execvp() が成功すれば処理は戻らない */

	execvp(argv[0], argv);

LIBFUNC_COMMIT

LIBFUNC_FINALIZE
}

static int child_write_dup_exec_exit(const char* quename, struct sfq_value* val)
{
LIBFUNC_INITIALIZE

	int irc = 0;

	char* execpath = "/bin/sh";
	char* execargs = NULL;
	int* pipefd = NULL;

	char env_ulong[32] = "";

/*
ヒープに入れたままだと exec() されたときにメモリリーク扱いになるので
payload 以外はスタックにコピー

payload は pipe 経由で送信
*/
	if (val->execpath)
	{
		if (val->execpath[0])
		{
			execpath = sfq_stradup(val->execpath);
			if (! execpath)
			{
				FIRE(SFQ_RC_ES_MEMALLOC, "execpath");
			}
		}
	}

	if (val->execargs)
	{
		if (val->execargs[0])
		{
			execargs = sfq_stradup(val->execargs);
			if (! execargs)
			{
				FIRE(SFQ_RC_ES_MEMALLOC, "execargs");
			}
		}
	}

	if (val->payload && val->payload_size)
	{
		size_t wn = 0;

		/* alloc fd x 2 (READ, WRITE) */
		pipefd = alloca(sizeof(int) * 2);
		if (! pipefd)
		{
			FIRE(SFQ_RC_ES_MEMALLOC, "pipefd");
		}

		pipefd[0] = pipefd[1] = 0;
		irc = pipe(pipefd);

		if (irc == -1)
		{
			FIRE(SFQ_RS_ES_PIPE, "pipefd");
		}

		wn = atomic_write(pipefd[WRITE], (char*)val->payload, val->payload_size);

		if (wn != val->payload_size)
		{
			FIRE(SFQ_RS_ES_WRITE, "atomic_write pipefd[WRITE]");
		}

		close(pipefd[WRITE]);

		irc = dup2(pipefd[READ], STDIN_FILENO);

		if (irc == -1)
		{
			FIRE(SFQ_RS_ES_DUP, "pipefd[READ] -> STDIN_FILENO");
		}
	}

	if (quename)
	{
		setenv("SFQ_QUENAME", quename, 0);
	}

	/* id */
	snprintf(env_ulong, sizeof(env_ulong), "%zu", val->id);
	setenv("SFQ_ID", env_ulong, 0);

	/* pushtime */
	snprintf(env_ulong, sizeof(env_ulong), "%zu", val->pushtime);
	setenv("SFQ_PUSHTIME", env_ulong, 0);

	if (val->metadata)
	{
		if (val->metadata[0])
		{
			setenv("SFQ_META", val->metadata, 0);
		}
	}

	sfq_free_value(val);

/* */
	execapp(execpath, execargs);

/*
exec() が成功すればここには来ない
*/
	FIRE(SFQ_RC_EC_EXECFAIL, "execapp");

LIBFUNC_COMMIT

	if (pipefd)
	{
		close(pipefd[READ]);
		close(pipefd[WRITE]);
	}

LIBFUNC_FINALIZE

	return LIBFUNC_RC();
}

static int pipe_fork_write_dup_exec_wait(const char* querootdir, const char* quename,
	struct sfq_value* val)
{
	int irc = 0;
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
			default_reopen(om ? om->queexeclogdir : NULL);
#endif
			om_quename = sfq_stradup(om->quename);
			sfq_free_open_names(om);

			child_write_dup_exec_exit(om_quename, val);

			sfq_free_value(val);

/*
exec() が成功すればここには来ない
*/
			exit (SFQ_RC_EC_EXECFAIL);
		}
		else
		{
/* parent */
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

static void foreach_element(const char* querootdir, const char* quename, int slotno)
{
LIBFUNC_INITIALIZE

	bool b = false;
	int irc = 0;

	/* 状態を LOOPSTART に変更 */
	b = update_procstatus(querootdir, quename, slotno, SFQ_PIS_LOOPSTART, 0);
	if (! b)
	{
		FIRE(SFQ_RC_EA_UPDSTATUS, "loop start");
	}

	do
	{
		struct sfq_value val;
		int to_status = SFQ_TO_NONE;

		bzero(&val, sizeof(val));

		irc = sfq_shift(querootdir, quename, &val);
		if (irc == SFQ_RC_NO_ELEMENT)
		{
			/* no more element */

			break;
		}

		if (irc == SFQ_RC_SUCCESS)
		{
			irc = pipe_fork_write_dup_exec_wait(querootdir, quename, &val);
			if (irc == 0)
			{
				/* execapp() exit(== 0) */
				to_status = SFQ_TO_SUCCESS;
			}
			else if (irc == SFQ_RC_EC_EXECFAIL)
			{
				to_status = SFQ_TO_CANTEXEC;
			}
			else if (irc > 0)
			{
				/* execapp() exit(<> 0) */
/*
1 - 127 のユーザが使える exit-code
*/
				to_status = SFQ_TO_APPEXIT_NON0;
			}
			else
			{
/* 不明 */
				/* can not execapp() */
				to_status = SFQ_TO_CANTEXEC;
			}
		}
		else
		{
/* shift 失敗 */
			/* shift error (<> not found) */
			to_status = SFQ_TO_FAULT;
		}

		sfq_free_value(&val);

		update_procstatus(querootdir, quename, slotno, SFQ_PIS_TAKEOUT, to_status);
	}
	while (irc == SFQ_RC_SUCCESS);

LIBFUNC_COMMIT

	update_procstatus(querootdir, quename, slotno, SFQ_PIS_DONE, 0);

LIBFUNC_FINALIZE
}

bool sfq_go_exec(const char* querootdir, const char* quename, int slotno)
{
	pid_t pid = (pid_t)-1;

#ifdef SFQ_DEBUG_BUILD
	assert(slotno >= 0);
#endif

	// 子プロセスを wait() しない
	signal(SIGCHLD, SIG_IGN);

	pid = fork();
	if (pid < 0)
	{
		/* fault create new-process */
		return false;
	}

	if (pid == 0) 
	{
/* child process */
		struct sfq_open_names* om = NULL;
		om = sfq_alloc_open_names(querootdir, quename);
#if STDIO_REOPEN_LOGFILE
		default_reopen(om ? om->queproclogdir : NULL);
#endif
		sfq_free_open_names(om);

		foreach_element(querootdir, quename, slotno);

		exit (EXIT_SUCCESS);
	}
	else
	{
/* parent process */
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
	int slotno, sfq_uchar procstatus, int to_status)
{
LIBFUNC_INITIALIZE

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
		FIRE(SFQ_RC_EA_OPENFILE, "sfq_open_queue");
	}

/* read file-header */
	b = sfq_readqfh(qo->fp, &qfh, &procs);
	if (! b)
	{
		FIRE(SFQ_RC_EA_READQFH, "sfq_readqfh");
	}

	if (qfh.qh.sval.max_proc_num <= slotno)
	{
		FIRE(SFQ_RC_EA_FUNCARG, "qfh.qh.sval.max_proc_num <= i");
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
		FIRE(SFQ_RC_EA_SEEKSETIO, "sfq_seek_set_and_write");
	}

LIBFUNC_COMMIT

	sfq_close_queue(qo);
	qo = NULL;

	free(procs);
	procs = NULL;

LIBFUNC_FINALIZE

	return LIBFUNC_IS_SUCCESS();
}

#if STDIO_REOPEN_LOGFILE
/*
http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
*/
static bool mkdir_p(const char *arg, mode_t mode)
{
	char* dir = NULL;
	char *p = NULL;
	size_t len = 0;

	struct stat stbuf;

	if (stat(arg, &stbuf) == 0)
	{
		return true;
	}

	dir = sfq_stradup(arg);
	if (! dir)
	{
		return false;
	}

	len = strlen(dir);
	if (dir[len - 1] == '/')
	{
		dir[len - 1] = 0;
	}

	for(p = dir+1; *p; p++)
	{
		if(*p == '/')
		{
			*p = '\0';
			if (mkdir(dir, mode) == -1)
			{
				if (errno != EEXIST)
				{
					return false;
				}
			}
			*p = '/';
		}
	}

	if (mkdir(dir, mode) == -1)
	{
		if (errno != EEXIST)
		{
			return false;
		}
	}

	return true;
}

static void default_reopen(const char* logdir)
{
	char* soutpath = NULL;
	char* serrpath = NULL;

	if (logdir)
	{
		time_t now = time(NULL);
		pid_t ppid = getppid();
		pid_t pid = getpid();

		struct tm tmbuf;
		char dir[PATH_MAX] = "";

		localtime_r(&now, &tmbuf);

		/* "QLDIR/yyyy/mmdd/hh/ppid-pid.{out,err}" */

		/* stdout */
		snprintf(dir, sizeof(dir), "%s/%04d/%02d%02d/%02d/%02d/%02d",
			logdir, tmbuf.tm_year + 1900, tmbuf.tm_mon + 1, tmbuf.tm_mday, tmbuf.tm_hour,
			tmbuf.tm_min, tmbuf.tm_sec);

		if (mkdir_p(dir, 0700))
		{
			/* "/proc/sys/kernel/pid_max" ... プロセス番号最大 */

			/* "dir/ppid-pid.{out,err}\0" */
			size_t alloc_size = (strlen(dir) + 1 + 20 + 1 + 20 + 1 + 3 + 1);

			soutpath = alloca(alloc_size);
			if (soutpath)
			{
				snprintf(soutpath, alloc_size, "%s/%d-%d.out", dir, ppid, pid);
			}
			serrpath = alloca(alloc_size);
			if (serrpath)
			{
				snprintf(serrpath, alloc_size, "%s/%d-%d.err", dir, ppid, pid);
			}
		}
	}

	if (! soutpath)
	{
		soutpath = "/dev/null";
	}

	if (! serrpath)
	{
		serrpath = "/dev/null";
	}

	/* stdio */
	freopen("/dev/null", "rb", stdin);

	/* stdout */
	if (! freopen(soutpath, "wb", stdout))
	{
		freopen("/dev/null", "wb", stdout);
	}

	/* stderr */
	if (! freopen(serrpath, "wb", stderr))
	{
		freopen("/dev/null", "wb", stderr);
	}
}
#endif

