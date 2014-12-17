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
typedef long			ssize_t;
typedef int			mode_t;
typedef int			uid_t;
typedef int			gid_t;
typedef int			cap_value_t;

#define true			(1)
#define false			(0)

#define PATH_MAX		(99)
#define _SC_ARG_MAX		(99)
#define _SC_CHILD_MAX		(99)
#define SEM_FAILED		(99)
#define MAXNAMLEN		(99)

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
#define optind			(99)

#define strerror_r(a, b, c)
#define uuid_copy(a, b)
#define uuid_unparse(a, b)
#define uuid_generate_random(a)
#define dirname(a)		(99)

#define CAP_CHOWN		(99)

#define S_IRUSR			(99)
#define S_IWUSR			(99)
#define S_IXUSR			(99)
#define S_ISGID			(99)
#define S_IRGRP			(99)
#define S_IWGRP			(99)
#define S_IXGRP			(99)

#define SIGHUP			(99)
#define SIGQUIT			(99)

#define umask(a)		(99)
#define chdir(a)		(99)
#define setsid()		(99)
#define chown(a)		(99)
#define geteuid(a)		(99)
#define getegid(a)		(99)

#define getpwnam_r(a, b, c, d, e)	(99)
#define getgrnam_r(a, b, c, d, e)	(99)
#define fchmod(a, b)			(99)
#define gettimeofday(a, b)		(99)

struct passwd { int pw_uid; };
struct group { int gr_gid; };
struct timeval { int tv_sec; };

typedef void (*sighandler_t)(int);

#define __func__		"__func__"


#ifdef __cplusplus
}
#endif

#endif

