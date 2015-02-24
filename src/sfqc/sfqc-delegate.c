#include "sfqc-lib.h"
#include <signal.h>
#include <sys/wait.h>

static struct sfqc_program_args pgargs;

static void release_heap()
{
	sfqc_free_program_args(&pgargs);
}

#define READ			(0)
#define WRITE			(1)
#define EXECARG_DELIM		'\t'

static volatile sfq_bool GLOBAL_child_down = SFQ_false;
static volatile sfq_bool GLOBAL_parent_down = SFQ_false;
static pid_t GLOBAL_writer_pid = (pid_t)-1;

static void sighandler_child_down(int signo)
{
	signal(signo, SIG_IGN);
	GLOBAL_child_down = SFQ_true;
	signal(signo, SIG_DFL);
}

static int kill_writer()
{
	int irc = -1;

	if (GLOBAL_writer_pid == (pid_t)-1)
	{
/* already killed */
		return 10;
	}

	irc = kill(GLOBAL_writer_pid, SIGTERM);
	if (irc == 0)
	{
		GLOBAL_writer_pid = (pid_t)-1;
	}

	return irc;
}

static void sighandler_parent_down(int signo)
{
	signal(signo, SIG_IGN);
	GLOBAL_parent_down = SFQ_true;
	kill_writer();
	signal(signo, SIG_DFL);
}

static void delegate_io_process()
{
	int irc = -1;

	pid_t pid = (pid_t)-1;
	int pipefd[] = { 0, 0 };

	char* line = NULL;
	size_t line_siz = 0;

/* */
	signal(SIGCHLD, sighandler_child_down);

	irc = pipe(pipefd);
	if (irc == -1)
	{
		goto EXIT_LABEL;
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
		signal(SIGCHLD, SIG_DFL);

		umask(0);
		chdir("/");

		close(pipefd[READ]);
		dup2(pipefd[WRITE], STDOUT_FILENO);
/*
#		setvbuf(stdout, NULL, _IONBF, 0);
*/
		sfq_execapp(pgargs.d_execpath, pgargs.d_execargs);

		close(pipefd[WRITE]);

		exit (SFQ_RC_EC_EXECFAIL);
	}
	else
	{
/* parent */
		char buff[BUFSIZ] = { '\0' };
		size_t buff_siz = 0;
		int status = -1;

/* */
		GLOBAL_writer_pid = pid;
		signal(SIGTERM, sighandler_parent_down);

		close(pipefd[WRITE]);
		dup2(pipefd[READ], STDIN_FILENO);

		buff_siz = sizeof(buff);

		while ((! GLOBAL_child_down) && fgets(buff, buff_siz, stdin))
		{
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
				}

				strcpy(&line[line_len], buff);
			}

			if (chg)
			{
				uuid_t uuid;

				uuid_clear(uuid);

				irc = sfq_push_text(pgargs.querootdir, pgargs.quename,
					pgargs.eworkdir, pgargs.execpath, pgargs.execargs,
					pgargs.metatext, pgargs.soutpath, pgargs.serrpath,
					uuid,
					line);

				if (irc != SFQ_RC_SUCCESS)
				{
					break;
				}
puts("pushed");

				/* reset buffer */
				line[0] = '\0';
			}
		}
puts("end loop");

		irc = kill_writer();
printf("kill_writer() --> %d\n", irc);

		irc = waitpid(pid, &status, 0);
printf("waitpid(%d) --> %d\n", pid, irc);

		if (irc != -1)
		{
puts("waitpid");
			if (WIFEXITED(status))
			{
				int exno = WEXITSTATUS(status);
printf("W exit (%d)\n", exno);
			}
			else if (WIFSIGNALED(status))
			{
				int signo = WTERMSIG(status);
printf("W signal (%d)\n", signo);
			}
		}

		if (GLOBAL_child_down)
		{
puts("child dead");
		}

		if (GLOBAL_parent_down)
		{
puts("parent dead");
		}

		close(pipefd[READ]);
	}

	irc = 0;

EXIT_LABEL:

	free(line);
	line = NULL;

	return exit(irc);
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
	irc = sfqc_parse_program_args(argc, argv, "D:N:X:A:w:o:e:x:a:m:q", SFQ_false, &pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	sfq_set_print(pgargs.quiet ? SFQ_false : SFQ_true);

	if (! pgargs.d_execpath)
	{
		message = "specify the path to become data source";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

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

