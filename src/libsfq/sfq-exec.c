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

#define READ			(0)
#define WRITE			(1)
#define EXECARG_DELIM		'\t'

static void execapp(const char* execpath, char* execargs)
{
SFQ_LIB_ENTER

	int argc = 0;
	char** argv = NULL;
	size_t argv_size = 0;

	int valnum = 0;

	if (execargs)
	{
/*
(カンマ区切りの) 引数の数を数える

--> カンマの数 + 1 = 値の数
*/
		valnum = 1 + sfq_count_char(EXECARG_DELIM, execargs);
	}

/*
argv = [ execpath, execargs[0], execargs[1], ..., execargs[n], NULL ]

--> count(execargs) + 2
*/
	argc = valnum + 2;
	argv_size = (sizeof(char*) * argc);

	argv = alloca(argv_size);
	if (! argv)
	{
		SFQ_FAIL(ES_MEMORY, "argv");
	}
	bzero(argv, argv_size);

/* set argv */
	argv[0] = (char*)execpath;

	if (execargs)
	{
		char delimiter_str[] = { EXECARG_DELIM, '\0' };

		char* pos = NULL;
		char* saveptr = NULL;
		char* token = NULL;
		int i = 0;

		for (i=1, pos=execargs; ; i++, pos=NULL)
		{
			token = strtok_r(pos, delimiter_str, &saveptr);
			if (! token)
			{
				break;
			}

			if ((i + 1) >= argc)
			{
				break;
			}

			argv[i] = token;
		}
	}

/*
execvp() が成功すれば処理は戻らない
*/

	execvp(argv[0], argv);

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE
}

static void output_reopen_4exec(const char* logdir, const struct sfq_value* val, mode_t dir_perm, mode_t file_perm)
{
	time_t now = time(NULL);

	const char* soutpath = val->soutpath;
	const char* serrpath = val->serrpath;

	if (soutpath && serrpath)
	{
		if (strcmp(soutpath, serrpath) == 0)
		{
			if (strcmp(soutpath, "-") != 0)
			{
/*
標準出力先と標準エラー出力先が同じファイル名
*/
elog_print("\tsoutpath == serrpath [%s], redirect stderr to /dev/null", soutpath);

				serrpath = NULL;
			}
		}
	}

/* stdout */
	sfq_output_reopen_4exec(stdout, &now,
		soutpath, logdir, val->uuid, val->id, "out", "SFQ_SOUTPATH", dir_perm, file_perm);

/* stderr */
	sfq_output_reopen_4exec(stderr, &now,
		serrpath, logdir, val->uuid, val->id, "err", "SFQ_SERRPATH", dir_perm, file_perm);
}

#define PATH_DELIM	':'

static int find_path(const char* needle, const char* arg_haystack)
{
	int ret = -1;
	char* haystack = NULL;

	char delimiter_str[] = { PATH_DELIM, '\0' };

	char* pos = NULL;
	char* saveptr = NULL;
	char* token = NULL;
	int i = 0;

SFQ_LIB_ENTER

	if ((! needle) || (! arg_haystack))
	{
		SFQ_FAIL(EA_FUNCARG, "arg is null");
	}

	haystack = sfq_stradup(arg_haystack);
	if (! haystack)
	{
		SFQ_FAIL(ES_MEMORY, "sfq_stradup(arg_haystack)");
	}

	for (i=0, pos=haystack; ; i++, pos=NULL)
	{
		char* cmppath = NULL;

		token = strtok_r(pos, delimiter_str, &saveptr);
		if (! token)
		{
			break;
		}

/* check path */
		cmppath = realpath(token, NULL);
		if (cmppath)
		{
			if (strcmp(cmppath, needle) == 0)
			{
				ret = 1;
			}

			free(cmppath);
			cmppath = NULL;
		}

		if (ret == 1)
		{
			break;
		}
	}

	if (ret != 1)
	{
		ret = 0;
	}

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	return ret;
}

static int child_write_dup_exec_exit(const struct sfq_eloop_params* elop, struct sfq_value* val)
{
SFQ_LIB_ENTER
	const char* eworkdir = NULL;
	const char* execpath = "/bin/sh";
	char* execargs = NULL;

	int irc = -1;
	int* pipefd = NULL;

	char* env_ptr = NULL;
	char env_ulong[32] = "";
	char uuid_s[36 + 1] = "";

/*
ヒープに入れたままだと exec() されたときにメモリリーク扱いになるので
payload 以外はスタックにコピー

payload は pipe 経由で送信
*/
elog_print("\tprepare exec");

	if (val->eworkdir)
	{
		eworkdir = val->eworkdir;
	}
	else
	{
		eworkdir = getenv("SFQ_EWORKDIR");
		if (! eworkdir)
		{
			eworkdir = "/";
		}
	}

	irc = chdir(eworkdir);
	if (irc != 0)
	{
		SFQ_FAIL(ES_PATH, "change dir to '%s'", eworkdir);
	}
elog_print("\t\tchdir = %s", eworkdir);

	if (strcmp(eworkdir, "/") != 0)
	{
		const char* cur_path = getenv("PATH");
		if (cur_path)
		{
			irc = find_path(eworkdir, cur_path);
			if (irc < 0)
			{
				SFQ_FAIL(EA_ISINPATH, "find_path(%s, %s)", eworkdir, cur_path);
			}

			if (irc)
			{
elog_print("\t\tpath = %s (not add)", cur_path);
			}
			else
			{
/*
現在の PATH 環境変数中に eworkdir が存在しない場合

--> PATH 環境変数に eworkdir を追加する
*/
				const char* new_path = sfq_concat(eworkdir, ":", cur_path);
				if (! new_path)
				{
					SFQ_FAIL(ES_MEMORY, "sfq_concat");
				}

				setenv("PATH", new_path, 1);
elog_print("\t\tpath = %s (add)", new_path);
			}
		}
		else
		{
			setenv("PATH", eworkdir, 1);
elog_print("\t\tpath = %s (set)", eworkdir);
		}
	}

	if (val->execpath)
	{
		execpath = sfq_stradup(val->execpath);
		if (! execpath)
		{
			SFQ_FAIL(ES_MEMORY, "execpath");
		}
elog_print("\t\texecpath = %s", execpath);
	}

	if (val->execargs)
	{
		execargs = sfq_stradup(val->execargs);
		if (! execargs)
		{
			SFQ_FAIL(ES_MEMORY, "execargs");
		}
elog_print("\t\texecargs = %s", execargs);
	}

	if (val->payload && val->payload_size)
	{
		int irc = 0;
		size_t wn = 0;

		/* alloc fd x 2 (READ, WRITE) */
		pipefd = alloca(sizeof(int) * 2);
		if (! pipefd)
		{
			SFQ_FAIL(ES_MEMORY, "pipefd");
		}

		pipefd[0] = pipefd[1] = 0;
		irc = pipe(pipefd);

		if (irc == -1)
		{
			SFQ_FAIL(ES_FORK, "pipefd");
		}

		wn = atomic_write(pipefd[WRITE], (char*)val->payload, val->payload_size);

		if (wn != val->payload_size)
		{
			SFQ_FAIL(ES_FORK, "atomic_write pipefd[WRITE]");
		}

		close(pipefd[WRITE]);

		irc = dup2(pipefd[READ], STDIN_FILENO);

		if (irc == -1)
		{
			SFQ_FAIL(ES_FORK, "pipefd[READ] -> STDIN_FILENO");
		}

elog_print("\t\tpayload size = %zu", val->payload_size);
	}

	/* exec loop */
	env_ptr = getenv("SFQ_EXECLOOP");
	if (env_ptr)
	{
		char* e = NULL;
		ulong ul = strtoul(env_ptr, &e, 0);

		ul++;

		snprintf(env_ulong, sizeof(env_ulong), "%zu", ul);
		setenv("SFQ_EXECLOOP", env_ulong, 1);
	}
	else
	{
		setenv("SFQ_EXECLOOP", "1", 1);
	}

	/* que dir */
	setenv("SFQ_QUEROOTDIR", elop->om_querootdir, 1);
elog_print("\t\tquerootdir = %s", elop->om_querootdir);

	/* queue */
	setenv("SFQ_QUENAME", elop->om_quename, 1);
elog_print("\t\tquename = %s", elop->om_quename);

	/* work dir */
	setenv("SFQ_EWORKDIR", eworkdir, 1);
elog_print("\t\teworkdir = %s", eworkdir);

	/* id */
	snprintf(env_ulong, sizeof(env_ulong), "%zu", val->id);
	setenv("SFQ_ID", env_ulong, 1);
elog_print("\t\tid = %zu", val->id);

	/* pushtime */
	snprintf(env_ulong, sizeof(env_ulong), "%zu", val->pushtime);
	setenv("SFQ_PUSHTIME", env_ulong, 1);
elog_print("\t\tpushtime = %zu", val->pushtime);

	/* uuid */
	uuid_unparse(val->uuid, uuid_s);
	setenv("SFQ_UUID", uuid_s, 1);
elog_print("\t\tuuid = %s", uuid_s);

	/* metatext */
	if (val->metatext)
	{
		setenv("SFQ_META", val->metatext, 1);

elog_print("\t\tmetatext = %s", val->metatext);
	}

/* */
	output_reopen_4exec(elop->om_queexeclogdir, val, elop->dir_perm, elop->file_perm);

	sfq_free_value(val);

/* */
	execapp(execpath, execargs);

/*
exec() が成功すればここには来ない
*/

SFQ_LIB_CHECKPOINT

	if (pipefd)
	{
		close(pipefd[READ]);
		close(pipefd[WRITE]);
	}

SFQ_LIB_LEAVE

	return SFQ_LIB_RC();
}

int sfq_execwait(const struct sfq_eloop_params* elop, struct sfq_value* val)
{
	pid_t pid = (pid_t)-1;

	pid = fork();

	if (pid == 0)
	{
/* child */
/*
親プロセスで無視していたシグナルを元に戻す
*/
		signal(SIGHUP,  SIG_DFL);
		signal(SIGINT,  SIG_DFL);
		signal(SIGQUIT, SIG_DFL);

		umask(0);
/*
		chdir("/");
*/

		child_write_dup_exec_exit(elop, val);

		sfq_free_value(val);

/*
exec() が成功すればここには来ない
*/
		exit (SFQ_RC_EC_EXECFAIL);
	}
	else if (pid > 0)
	{
/* parent */
		int irc = 0;
		int status = 0;

elog_print("\tbefore wait child-process [pid=%d]", pid);

/*
子プロセスの終了を待つ
*/
		irc = waitpid(pid, &status, 0);
		if (irc != -1)
		{
			if (WIFEXITED(status))
			{
				int exit_code = WEXITSTATUS(status);

elog_print("\tafter wait child-process [pid=%d exit=%d]", pid, exit_code);
				sfq_write_execrc(elop->om_queexeclogdir, val->uuid, exit_code);

				return exit_code;
			}
		}
	}

	return -1;
}

