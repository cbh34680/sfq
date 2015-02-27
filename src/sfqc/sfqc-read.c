#include "sfqc-lib.h"
#include <errno.h>

static struct sfqc_program_args pgargs;

static void release_heap()
{
	sfqc_free_program_args(&pgargs);
}

#define READ			(0)
#define WRITE			(1)

static pid_t GLOBAL_writer_pid = (pid_t)-1;
static volatile sfq_bool GLOBAL_ready = SFQ_false;

static void log_print(const char* const format, ...)
{
	va_list ap;
	va_start(ap, format);

	fprintf(stderr, "pid=%d\t", getpid());
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");

	va_end(ap);
}

static void sighandler_parent_term(int signo)
{
	signal(signo, SIG_IGN);

/*
子プロセスに SIGTERM を送り終了させることで、自プロセスの fgets を終了させる
*/
	kill(GLOBAL_writer_pid, SIGTERM);

	signal(signo, SIG_DFL);
}

void sighandler_ready(int signo)
{
	signal(signo, SIG_DFL);

	GLOBAL_ready = SFQ_true;
}

static void exec_writer(int pipefd[2])
{
	const char* execpath = "/bin/sh";

	if (pgargs.d_execpath)
	{
		execpath = pgargs.d_execpath;
	}

	umask(0);
	chdir("/");

	close(pipefd[READ]);
	dup2(pipefd[WRITE], STDOUT_FILENO);

	freopen("/dev/null", "r", stdin);

log_print("exec path=%s", execpath);

	sfq_execapp(execpath, pgargs.d_execargs);

	close(pipefd[WRITE]);
}

static void delegate_io_process()
{
	int irc = -1;

	pid_t pid = (pid_t)-1;
	int pipefd[] = { 0, 0 };

	char* line = NULL;
	size_t line_siz = 0;

	int lno = 0;
	char buff[BUFSIZ] = { '\0' };
	size_t buff_siz = sizeof(buff);

	int status = -1;

/* */
	irc = pipe(pipefd);
	if (irc == -1)
	{
		goto EXIT_LABEL;
	}

	if (pgargs.d_serrpath)
	{
		if (freopen(pgargs.d_serrpath, "at", stderr))
		{
/*
			ftruncate(fileno(stderr), 0);
*/

			setvbuf(stderr, NULL, _IONBF, 0);
		}
		else
		{
			freopen("/dev/null", "wt", stderr);
		}
	}
	else
	{
		freopen("/dev/null", "wt", stderr);
	}


	pid = fork();
	if (pid < 0)
	{
/* error */
		goto EXIT_LABEL;
	}

	if (pid == 0)
	{
/* child */
		exec_writer(pipefd);

		exit (SFQ_RC_EC_EXECFAIL);
	}

/* parent */

/* */
	GLOBAL_writer_pid = pid;

	close(pipefd[WRITE]);
	dup2(pipefd[READ], STDIN_FILENO);
	freopen("/dev/null", "a", stdout);

log_print("wait for prepare child process ...");
	sleep(3);

	signal(SIGTERM, sighandler_parent_term);
	signal(SIGALRM, sighandler_ready);
	alarm(3);

log_print("set skip first alarm, enter loop");

	while (fgets(buff, buff_siz, stdin))
	{
/*
変更された (chg != 0) --> 改行があった
*/
		size_t chg = sfq_rtrim(buff, "\n");
		size_t buff_len = strlen(buff);

		if (buff_len)
		{
			size_t line_len = 0;
			if (line)
			{
				line_len = strlen(line);
			}

			size_t next_siz = line_len + buff_len + 1;

			if (next_siz > line_siz)
			{
				line_siz = next_siz;

				line = realloc(line, line_siz);
				if (! line)
				{
log_print("allocate memory fault");
					break;
				}
			}

/*
strcat() と同義
*/
			strcpy(&line[line_len], buff);
		}

		if (chg)
		{
/*
改行がある場合 -> 行の読み込み完了
*/
			if (GLOBAL_ready)
			{
				uuid_t uuid;
				char uuid_s[36 + 1] = { '\0' };

				uuid_clear(uuid);


				irc = sfq_push_text(pgargs.querootdir, pgargs.quename,
					pgargs.eworkdir, pgargs.execpath, pgargs.execargs,
					pgargs.metatext, pgargs.soutpath, pgargs.serrpath,
					uuid,
					line);

				uuid_unparse(uuid, uuid_s);

log_print("%d) push(%s) line=[%s]", lno, uuid_s, line);

				if (irc != SFQ_RC_SUCCESS)
				{
log_print("push operation return error=%d", irc);

					break;
				}

				/* reset buffer */
				line[0] = '\0';

				lno++;
			}
		}
	}

log_print("break loop");

/*
子プロセスの後始末
*/
	kill(pid, SIGTERM);

log_print("wait for stop pid=%d", pid);
	irc = waitpid(pid, &status, 0);

	if (irc == -1)
	{
log_print("wait pid=%d fail, errno=%d", pid, errno);
	}
	else
	{
		const char* cause = "unknown";
		int code = -1;

		if (WIFEXITED(status))
		{
			cause = "exit";
			code = WEXITSTATUS(status);
		}
		else if (WIFSIGNALED(status))
		{
			cause = "signal";
			code = WTERMSIG(status);
		}

log_print("wait result cause[%s] code[%d]", cause, code);
	}

	close(pipefd[READ]);

log_print("all done.");

	irc = 0;

EXIT_LABEL:

	free(line);
	line = NULL;

	return exit((irc == 0) ? 0 : 1);
}

static int create_daemon()
{
	pid_t pid = (pid_t)-1;

	signal(SIGCHLD, SIG_IGN);

	pid = fork();

	if (pid == 0)
	{
/* child */
		signal(SIGCHLD, SIG_DFL);

		umask(0);
		chdir("/");

		delegate_io_process();
	}

	return (pid < 0) ? 1 : 0;
}

int main(int argc, char** argv)
{
	int irc = -1;
	const char* message = NULL;
	int jumppos = -1;

/* */
	atexit(release_heap);

SFQC_MAIN_ENTER

	bzero(&pgargs, sizeof(pgargs));

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:X:A:E:w:o:e:x:a:m:q", SFQ_false, &pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	sfq_set_print(pgargs.quiet ? SFQ_false : SFQ_true);

	irc = create_daemon();
	if (irc != 0)
	{
		message = "create daemon fault";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

EXIT_LABEL:

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

SFQC_MAIN_LEAVE

	return irc;
}

