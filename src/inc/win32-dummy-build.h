#ifndef SFMQ_WIN32_DUMMY_BUILD_INCLUDE_H_ONCE__
#define SFMQ_WIN32_DUMMY_BUILD_INCLUDE_H_ONCE__

#pragma warning( disable : 4819 4996 )

/*
I love VisulStudio.
force build by VS, but not work
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long		ulong;
typedef int			sem_t;
typedef int			pid_t;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned char		bool;
typedef long			ssize_t;
typedef int			mode_t;

#define true			(1)
#define false			(0)

#define PATH_MAX		(255)
#define _SC_ARG_MAX		(99)
#define _SC_CHILD_MAX		(99)
#define SEM_FAILED		(99)

#define bzero(a, b)
#define sysconf(a)		(99)
#define getpid()		(99)
#define getppid()		(99)
#define realpath(a, b)		NULL
#define snprintf(...)
#define alloca(a)		NULL

#define sem_open(a, b, c, d)	(99)
#define sem_wait(a)		(99)
#define sem_post(a)		(99)
#define sem_close(a)		(99)
#define sem_unlink(a)		(99)

#define mkdir(a, b)		(99)
#define close(a)		(99)
#define dup2(a, b)		(99)
#define localtime_r(a, b)	(99)
#define setenv(a, b, c)		(99)
#define fork()			(99)
#define waitpid(a, b, c)	(99)
#define pipe(a)			(99)
#define write(a, b, c)		(99)
#define execvp(a, b)		(99)
#define strtok_r(a, b, c)	NULL

#define WIFEXITED(a)		(99)
#define WEXITSTATUS(a)		(99)

#define STDIN_FILENO		(99)
#define SIGCHLD			(99)

#define fseeko(a, b, c)		(99)

#define getopt(a, b, c)		(99)
#define optarg			NULL


#ifdef __cplusplus
}
#endif

#endif

