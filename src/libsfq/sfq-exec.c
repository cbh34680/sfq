#include "sfq-lib.h"

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

static void output_reopen_4exec(const char* logdir, const struct sfq_value* val)
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
}

static int child_write_dup_exec_exit(const char* om_quename,
		ushort slotno, const char* om_queexeclogdir, struct sfq_value* val)
{
SFQ_LIB_INITIALIZE

	char* execpath = "/bin/sh";
	char* execargs = NULL;
	int* pipefd = NULL;

	char env_ulong[32] = "";
	char uuid_s[36 + 1] = "";

/* */
fprintf(stderr, "\tprepare exec\n");

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
fprintf(stderr, "\t\texecpath = %s\n", execpath);
	}

	if (val->execargs)
	{
		execargs = sfq_stradup(val->execargs);
		if (! execargs)
		{
			SFQ_FAIL(ES_MEMALLOC, "execargs");
		}
fprintf(stderr, "\t\texecargs = %s\n", execargs);
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

fprintf(stderr, "\t\tpayload size = %zu\n", val->payload_size);
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

fprintf(stderr, "\t\tmetadata = %s\n", val->metadata);
	}

/* */
	output_reopen_4exec(om_queexeclogdir, val);

	sfq_free_value(val);

/* */
	execapp(execpath, execargs);

/*
exec() が成功すればここには来ない
*/
	//SFQ_FAIL(EC_EXECFAIL, "execapp");

SFQ_LIB_CHECKPOINT

	if (pipefd)
	{
		close(pipefd[READ]);
		close(pipefd[WRITE]);
	}

SFQ_LIB_FINALIZE

	return SFQ_LIB_RC();
}

int sfq_execwait(const char* om_querootdir, const char* om_quename,
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
			child_write_dup_exec_exit(om_quename, slotno, om_queexeclogdir, val);

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

