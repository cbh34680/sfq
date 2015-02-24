#include "sfqc-lib.h"
#include <signal.h>

static struct sfqc_program_args pgargs;

static void release_heap()
{
	sfqc_free_program_args(&pgargs);
}

#define READ			(0)
#define WRITE			(1)
#define EXECARG_DELIM		'\t'

static int delegate_io_process()
{
	int irc = -1;

	pid_t pid = (pid_t)-1;
	int pipefd[] = { 0, 0 };

	char* line = NULL;
	size_t line_siz = 0;

/* */
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
		char buff[10] = { '\0' };
		size_t buff_siz = 0;

printf("parent %d\n", getpid());

		close(pipefd[WRITE]);
		dup2(pipefd[READ], STDIN_FILENO);

		buff_siz = sizeof(buff);

		while (fgets(buff, buff_siz, stdin))
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
puts("grow");
					line_siz = next_siz;
					line = realloc(line, line_siz);
				}

				strcpy(&line[line_len], buff);
			}

			if (chg)
			{
				printf("get string = [%s] [%zu]\n", line, strlen(line));

				line[0] = '\0';
			}
		}

		close(pipefd[READ]);
	}

	irc = 0;

EXIT_LABEL:

	free(line);
	line = NULL;

	return irc;
}

static int create_daemon()
{
	pid_t pid = (pid_t)-1;

	signal(SIGCHLD, SIG_IGN);

	pid = fork();

	if (pid == 0)
	{
		int irc = -1;

/* child */
		signal(SIGCHLD, SIG_DFL);

		umask(0);
		chdir("/");

		irc = delegate_io_process();

		exit (irc);
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

