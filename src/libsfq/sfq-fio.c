#include "sfq-lib.h"

static sfq_bool seek_set_read_(FILE* fp, off_t set_pos, void* mem, size_t mem_size)
{
	int irc = 0;
	size_t iosize = 0;

	if (! fp)
	{
		return SFQ_false;
	}

	if (! mem)
	{
		return SFQ_false;
	}

	if (mem_size == 0)
	{
		return SFQ_false;
	}

	irc = fseeko(fp, set_pos, SEEK_SET);
	if (irc == -1)
	{
		return SFQ_false;
	}

	iosize = fread(mem, mem_size, 1, fp);
	if (iosize != 1)
	{
		return SFQ_false;
	}

	return SFQ_true;
}

static sfq_bool seek_set_write_(FILE* fp, off_t set_pos, const void* mem, size_t mem_size)
{
	int irc = 0;
	size_t iosize = 0;

	if (! fp)
	{
		return SFQ_false;
	}

	if (! mem)
	{
		return SFQ_false;
	}

	if (mem_size == 0)
	{
		return SFQ_false;
	}

	irc = fseeko(fp, set_pos, SEEK_SET);
	if (irc == -1)
	{
		return SFQ_false;
	}

	iosize = fwrite(mem, mem_size, 1, fp);
	if (iosize != 1)
	{
		return SFQ_false;
	}

	return SFQ_true;
}

static sfq_bool mkdir_withUGW(const char* dir, const struct sfq_queue_create_params* qcp)
{
	int irc = -1;

	mode_t dir_perm = (mode_t)-1;

SFQ_LIB_ENTER

	dir_perm = (S_IRUSR | S_IWUSR | S_IXUSR);
	if (qcp->chmod_GaW)
	{
		dir_perm |= (S_ISGID | S_IRGRP | S_IWGRP | S_IXGRP);
	}

	irc = mkdir(dir, dir_perm);
	if (irc != 0)
	{
		SFQ_FAIL(ES_MKDIR, "%s: make directory failed, check permission", dir);
	}

	if (SFQ_ISSET_UID(qcp->queusrid) || SFQ_ISSET_GID(qcp->quegrpid))
	{
		irc = chown(dir, qcp->queusrid, qcp->quegrpid);
		if (irc != 0)
		{
			SFQ_FAIL(ES_CHOWN, "chown");
		}
	}

/*
chown() を実行すると sticky-bit が外れるので
改めて chmod を実行する。

--> オプショナルな感があるので、失敗しても無視
*/

	chmod(dir, dir_perm);

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	return SFQ_LIB_IS_SUCCESS();
}


#ifdef SFQ_SEMUNLOCK_AT_SIGCATCH
/*
セマフォのロックを解除するシグナルハンドラ
*/
static void unlock_semaphore_sighandler(int signo)
{
	signal(signo, SIG_IGN);

	sfq_unlock_semaphore(NULL);
	raise(signo);

	signal(signo, SIG_DFL);
}
#endif

static struct sfq_queue_object* open_queue_(const char* querootdir, const char* quename,
	sfq_uchar queue_openmode, int semlock_wait_sec)
{
SFQ_LIB_ENTER

	struct sfq_queue_object* qo = NULL;
	struct sfq_open_names* om = NULL;
	FILE* fp = NULL;

	mode_t save_umask = (mode_t)-1;
	sfq_bool b = SFQ_false;
	const char* fopen_mode = NULL;

#ifdef SFQ_SEMUNLOCK_AT_SIGCATCH
	sighandler_t save_handler_SIGINT = NULL;
	sighandler_t save_handler_SIGTERM = NULL;
	sighandler_t save_handler_SIGHUP = NULL;
#endif

	assert(queue_openmode);

/* create names */
	om = sfq_alloc_open_names(querootdir, quename);
	if (! om)
	{
		SFQ_FAIL(EA_CREATENAMES, "sfq_alloc_open_names");
	}

	save_umask = umask(0);

/* lock semaphore */
	b = sfq_lock_semaphore(om->semname, semlock_wait_sec);
	if (! b)
	{
		SFQ_FAIL(EA_LOCKSEMAPHORE, "sfq_lock_semaphore");
	}

#ifdef SFQ_SEMUNLOCK_AT_SIGCATCH
/* regist signal handlers */
	/* SIGINT */
	save_handler_SIGINT = signal(SIGINT, unlock_semaphore_sighandler);
	if (save_handler_SIGINT == SIG_ERR)
	{
		SFQ_FAIL(ES_SIGNAL, "change SIGINT handler");
	}

	/* SIGTERM */
	save_handler_SIGTERM = signal(SIGTERM, unlock_semaphore_sighandler);
	if (save_handler_SIGTERM == SIG_ERR)
	{
		SFQ_FAIL(ES_SIGNAL, "change SIGTERM handler");
	}

	/* SIGHUP */
	save_handler_SIGHUP = signal(SIGHUP, unlock_semaphore_sighandler);
	if (save_handler_SIGHUP == SIG_ERR)
	{
		SFQ_FAIL(ES_SIGNAL, "change SIGHUP handler");
	}
#endif

/* open queue-file */
	switch (queue_openmode)
	{
		case (SFQ_FOM_READ | SFQ_FOM_WRITE):
		{
			fopen_mode = "rb+";
			break;
		}
		case SFQ_FOM_READ:
		{
			fopen_mode = "rb";
			break;
		}
		case SFQ_FOM_WRITE:
		{
			fopen_mode = "wb";
			break;
		}
	}

	assert(fopen_mode);

	fp = fopen(om->quefile, fopen_mode);
	if (! fp)
	{
		SFQ_FAIL(ES_FILE, "file open error '%s' (systemd[PrivateTmp=true] enable?)",
			om->quefile);
	}

/* */
	if (queue_openmode & SFQ_FOM_READ)
	{
		ssize_t siosize = 0; 
		struct sfq_file_stamp qfs;

		bzero(&qfs, sizeof(qfs));

		siosize = pread(fileno(fp), &qfs, sizeof(qfs), 0);
		if (siosize == -1)
		{
			SFQ_FAIL(ES_FILE, "FILE-READ(qfs)");
		}

/* check magic string */
		if (strncmp(qfs.magicstr, SFQ_MAGICSTR, strlen(SFQ_MAGICSTR)) != 0)
		{
			SFQ_FAIL(EA_ILLEGALVER, "magicstr");
		}

/* check data version */
		if (qfs.qfh_size != sizeof(struct sfq_file_header))
		{
			SFQ_FAIL(EA_ILLEGALVER, "sfq_file_header size not match");
		}
	}

/* create response */
	qo = malloc(sizeof(*qo));
	if (! qo)
	{
		SFQ_FAIL(ES_MEMORY, "malloc(locked_file)");
	}

	qo->om = om;
	qo->fp = fp;
	qo->opentime = time(NULL);
	qo->save_umask = save_umask;
	qo->queue_openmode = queue_openmode;

#ifdef SFQ_SEMUNLOCK_AT_SIGCATCH
	qo->save_handler_SIGINT = save_handler_SIGINT;
	qo->save_handler_SIGTERM = save_handler_SIGTERM;
	qo->save_handler_SIGHUP = save_handler_SIGHUP;
#endif

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		if (fp)
		{
			fclose(fp);
			fp = NULL;
		}

#ifdef SFQ_SEMUNLOCK_AT_SIGCATCH
/* un-regist signal handlers */
		/* SIGINT */
		if (save_handler_SIGINT)
		{
			if (save_handler_SIGINT != SIG_ERR)
			{
				signal(SIGINT, save_handler_SIGINT);
			}
			save_handler_SIGINT = NULL;
		}

		/* SIGTERM */
		if (save_handler_SIGTERM)
		{
			if (save_handler_SIGTERM != SIG_ERR)
			{
				signal(SIGTERM, save_handler_SIGTERM);
			}
			save_handler_SIGTERM = NULL;
		}

		/* SIGHUP */
		if (save_handler_SIGHUP)
		{
			if (save_handler_SIGHUP != SIG_ERR)
			{
				signal(SIGHUP, save_handler_SIGHUP);
			}
			save_handler_SIGHUP = NULL;
		}
#endif

		if (om)
		{
/* release semaphore */
			sfq_unlock_semaphore(om->semname);

			sfq_free_open_names(om);
			om = NULL;
		}

		if (save_umask != (mode_t)-1)
		{
			umask(save_umask);
		}

		free(qo);
		qo = NULL;
	}

SFQ_LIB_LEAVE

	return qo;
}

void sfq_close_queue(struct sfq_queue_object* qo)
{
	if (! qo)
	{
		return;
	}

	if (qo->fp)
	{
		fclose(qo->fp);
		qo->fp = NULL;
	}

#ifdef SFQ_SEMUNLOCK_AT_SIGCATCH
/* SIGINT */
	if (qo->save_handler_SIGINT)
	{
		if (qo->save_handler_SIGINT != SIG_ERR)
		{
			signal(SIGINT, qo->save_handler_SIGINT);
		}
		qo->save_handler_SIGINT = NULL;
	}

/* SIGTERM */
	if (qo->save_handler_SIGTERM)
	{
		if (qo->save_handler_SIGTERM != SIG_ERR)
		{
			signal(SIGTERM, qo->save_handler_SIGTERM);
		}
		qo->save_handler_SIGTERM = NULL;
	}

/* SIGHUP */
	if (qo->save_handler_SIGHUP)
	{
		if (qo->save_handler_SIGHUP != SIG_ERR)
		{
			signal(SIGHUP, qo->save_handler_SIGHUP);
		}
		qo->save_handler_SIGHUP = NULL;
	}
#endif

	if (qo->om)
	{
		sfq_unlock_semaphore(qo->om->semname);

		sfq_free_open_names(qo->om);
		qo->om = NULL;
	}

	if (qo->save_umask != (mode_t)-1)
	{
		umask(qo->save_umask);
		qo->save_umask = (mode_t)-1;
	}

	free(qo);
}

struct sfq_queue_object* sfq_open_queue_rw(const char* querootdir, const char* quename)
{
	return open_queue_(querootdir, quename, (SFQ_FOM_READ | SFQ_FOM_WRITE), 0);
}

struct sfq_queue_object* sfq_open_queue_rw_lws(const char* querootdir, const char* quename,
	int semlock_wait_sec)
{
	return open_queue_(querootdir, quename, (SFQ_FOM_READ | SFQ_FOM_WRITE),
		semlock_wait_sec);
}

struct sfq_queue_object* sfq_open_queue_ro(const char* querootdir, const char* quename)
{
	return open_queue_(querootdir, quename, SFQ_FOM_READ, 0);
}

struct sfq_queue_object* sfq_open_queue_ro_lws(const char* querootdir, const char* quename,
	int semlock_wait_sec)
{
	return open_queue_(querootdir, quename, SFQ_FOM_READ, semlock_wait_sec);
}

struct sfq_queue_object* sfq_open_queue_wo(const struct sfq_queue_create_params* qcp)
{
SFQ_LIB_ENTER

	struct sfq_queue_object* qo = NULL;
	struct sfq_open_names* om = NULL;

	mode_t file_perm = 0;
	sfq_bool b = SFQ_false;

	int irc = -1;
	struct stat stbuf;

/* */
	om = sfq_alloc_open_names(qcp->querootdir, qcp->quename);
	if (! om)
	{
		SFQ_FAIL(EA_CREATENAMES, "sfq_alloc_open_names");
	}

/* queue-rootdir not exists */
	irc = stat(om->querootdir, &stbuf);
	if (irc != 0)
	{
		/* already exists */
		SFQ_FAIL(EA_PATHNOTEXIST, "dir not exist '%s'", om->querootdir);
	}

/* queue-dir already exists */
	irc = stat(om->quedir, &stbuf);
	if (irc == 0)
	{
		/* already exists */
		SFQ_FAIL(EA_EXISTQUEUE, "dir already exist '%s'", om->quedir);
	}

/* force delete old lock */
	irc = sem_unlink(om->semname);
	if (irc == -1)
	{
		if (errno != ENOENT)
		{
			SFQ_FAIL(ES_FILE,
				"delete semaphore fault, check permission (e.g. /dev/shm%s)",
				om->semname);
		}
	}

/* make directory tree */
	b = mkdir_withUGW(om->quedir, qcp);
	if (! b)
	{
		SFQ_FAIL(ES_MKDIR, "quedir");
	}

	b = mkdir_withUGW(om->quelogdir, qcp);
	if (! b)
	{
		SFQ_FAIL(ES_MKDIR, "quelogdir");
	}

	b = mkdir_withUGW(om->queproclogdir, qcp);
	if (! b)
	{
		SFQ_FAIL(ES_MKDIR, "queproclogdir");
	}

	b = mkdir_withUGW(om->queexeclogdir, qcp);
	if (! b)
	{
		SFQ_FAIL(ES_MKDIR, "queexeclogdir");
	}

/* open queue (w) */
	qo = open_queue_(om->querootdir, om->quename, SFQ_FOM_WRITE, 0);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "queue open error name=[%s] file=[%s/%s]",
			om->quename, om->quedir, om->quefile);
	}

/* change owner/group and permission */

	file_perm = (S_IRUSR | S_IWUSR | S_IXUSR);
	if (qcp->chmod_GaW)
	{
		file_perm |= (S_IRGRP | S_IWGRP | S_IXGRP);
	}

	irc = chmod(qo->om->quefile, file_perm);
	if (irc != 0)
	{
		SFQ_FAIL(ES_CHMOD, "chmod");
	}

	if (SFQ_ISSET_UID(qcp->queusrid) || SFQ_ISSET_GID(qcp->quegrpid))
	{
		irc = chown(qo->om->quefile, qcp->queusrid, qcp->quegrpid);
		if (irc != 0)
		{
			SFQ_FAIL(ES_CHOWN, "chown");
		}
	}

SFQ_LIB_CHECKPOINT

	sfq_free_open_names(om);
	om = NULL;

	if (SFQ_LIB_IS_FAIL())
	{
		sfq_close_queue(qo);
		qo = NULL;
	}

SFQ_LIB_LEAVE

	return qo;
}

enum
{
	UPDATE_PREV_ELMPOS = 1,
	UPDATE_NEXT_ELMPOS = 2,
};

static sfq_bool update_elmpos_(FILE* fp, off_t seek_pos, int type, off_t updval)
{
	ssize_t siosize = 0;
	//sfq_bool b = SFQ_false;
	struct sfq_e_header eh;

	siosize = pread(fileno(fp), &eh, sizeof(eh), seek_pos);
	if (siosize == -1)
	{
		return SFQ_false;
	}

	switch (type)
	{
		case UPDATE_PREV_ELMPOS:
		{
			eh.prev_elmpos = updval;
			break;
		}

		case UPDATE_NEXT_ELMPOS:
		{

			eh.next_elmpos = updval;
			break;
		}

		default:
		{
			abort();
		}
	}

	siosize = pwrite(fileno(fp), &eh, sizeof(eh), seek_pos);
	if (siosize == -1)
	{
		return SFQ_false;
	}

	return SFQ_true;
}

sfq_bool sfq_unlink_prevelm(struct sfq_queue_object* qo, off_t seek_pos)
{
	/* prev_elmpos に 0 を設定し、リンクを切る */

	return update_elmpos_(qo->fp, seek_pos, UPDATE_PREV_ELMPOS, 0);
}

sfq_bool sfq_unlink_nextelm(struct sfq_queue_object* qo, off_t seek_pos)
{
	/* next_elmpos に 0 を設定し、リンクを切る */

	return update_elmpos_(qo->fp, seek_pos, UPDATE_NEXT_ELMPOS, 0);
}

sfq_bool sfq_link_nextelm(struct sfq_queue_object* qo, off_t seek_pos, off_t updval)
{
	return update_elmpos_(qo->fp, seek_pos, UPDATE_NEXT_ELMPOS, updval);
}

sfq_bool sfq_writeqfh(struct sfq_queue_object* qo, struct sfq_file_header* qfh,
	const struct sfq_process_info* procs, const char* lastoper)
{
/*
	sfq_bool b = SFQ_false;
	size_t iosize = 0;
*/
	ssize_t siosize = 0;

SFQ_LIB_ENTER

	if (! qo)
	{
		SFQ_FAIL(EA_FUNCARG, "qo");
	}

	if (lastoper)
	{
		strncpy(qfh->qh.dval.lastoper, lastoper, sizeof(qfh->qh.dval.lastoper) - 1);
	}

	qfh->qh.dval.update_cnt++;
	qfh->qh.dval.updatetime = qo->opentime;

/* */
	siosize = pwrite(fileno(qo->fp), qfh, sizeof(*qfh), 0);
	if (siosize != sizeof(*qfh))
	{
		SFQ_FAIL(EA_QFHRW, "write(qfh)");
	}
/*
	b = seek_set_write_(qo->fp, 0, qfh, sizeof(*qfh));
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "FILE-WRITE(qfh)");
	}
*/

	if (procs)
	{
		size_t procs_size = 0;
		size_t pi_size = 0;

		assert(qfh->qh.sval.procs_num > 0);

		pi_size = sizeof(struct sfq_process_info);
		procs_size = (pi_size * qfh->qh.sval.procs_num);

		siosize = pwrite(fileno(qo->fp), procs, procs_size, qfh->qh.sval.procseg_start_pos);
		if (siosize != procs_size)
		{
			SFQ_FAIL(ES_FILE, "FILE-WRITE(procs)");
		}
/*
		iosize = fwrite(procs, procs_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILE, "FILE-WRITE(procs)");
		}
*/
	}

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	return SFQ_LIB_IS_SUCCESS();
}

sfq_bool sfq_readqfh(struct sfq_queue_object* qo, struct sfq_file_header* qfh,
	struct sfq_process_info** procs_ptr)
{
	struct sfq_process_info* procs = NULL;
/*
	sfq_bool b = SFQ_false;
	size_t iosize = 0;
*/
	ssize_t siosize = 0;

SFQ_LIB_ENTER

/* */
	if (! qo)
	{
		SFQ_FAIL(EA_FUNCARG, "qo");
	}

	bzero(qfh, sizeof(*qfh));

/* read file-header */
/*
	b = seek_set_read_(qo->fp, 0, qfh, sizeof(*qfh));
	if (! b)
	{
		SFQ_FAIL(EA_SEEKSETIO, "seek_set_read_(qfh)");
	}
*/
	siosize = pread(fileno(qo->fp), qfh, sizeof(*qfh), 0);
	if (siosize != sizeof(*qfh))
	{
		SFQ_FAIL(EA_SEEKSETIO, "FILE-READ(qfh)");
	}

/* read process-table */
	if (procs_ptr)
	{
		if (qfh->qh.sval.procs_num)
		{
			size_t procs_size = 0;

			procs_size = qfh->qh.sval.procs_num * sizeof(struct sfq_process_info);
			procs = malloc(procs_size);

			if (! procs)
			{
				SFQ_FAIL(ES_MEMORY, "ALLOC(procs)");
			}

			siosize = pread(fileno(qo->fp), procs, procs_size,
				qfh->qh.sval.procseg_start_pos);

			if (siosize != procs_size)
			{
				SFQ_FAIL(ES_FILE, "FILE-READ(procs)");
			}
/*
			iosize = fread(procs, procs_size, 1, qo->fp);
			if (iosize != 1)
			{
				SFQ_FAIL(ES_FILE, "FILE-READ(procs)");
			}
*/

//#ifdef SFQ_DEBUG_BUILD
//			sfq_print_procs(procs, qfh->qh.sval.procs_num);
//#endif

			(*procs_ptr) = procs;
		}
	}

	if (qo->queue_openmode & SFQ_FOM_WRITE)
	{
        	qfh->last_qhd2 = qfh->last_qhd1;
        	qfh->last_qhd1 = qfh->qh.dval;
	}

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		free(procs);
		procs = NULL;

		bzero(qfh, sizeof(*qfh));
	}

SFQ_LIB_LEAVE

	return SFQ_LIB_IS_SUCCESS();
}

/*
 * element
 *    - header
 *    - payload    (eh.payload_size  >= 0)
 *    - eworkdir   (eh.eworkdir_size >= 0)   ... nullterm string
 *    - execpath   (eh.execpath_size >= 0)   ... nullterm string
 *    - execargs   (eh.execargs_size >= 0)   ... nullterm string
 *    - metatext   (eh.metatext_size >= 0)   ... nullterm string
 *    - soutpath   (eh.soutpath_size >= 0)   ... nullterm string
 *    - serrpath   (eh.serrpath_size >= 0)   ... nullterm string
 *
 */
#define WRITEELM_NTSTR(key_) \
	\
	if (ioeb->eh.key_ ## _size) \
	{ \
		assert(ioeb->key_); \
		if (fwrite(ioeb->key_, ioeb->eh.key_ ## _size, 1, qo->fp) != 1) \
		{ \
			SFQ_FAIL(ES_FILE, "fwrite"); \
		} \
	}


sfq_bool sfq_writeelm(struct sfq_queue_object* qo, off_t seek_pos, const struct sfq_ioelm_buff* ioeb)
{
	sfq_bool b = SFQ_false;
	size_t iosize = 0;

SFQ_LIB_ENTER

	if (! qo)
	{
		SFQ_FAIL(EA_FUNCARG, "qo");
	}

	b = seek_set_write_(qo->fp, seek_pos, &ioeb->eh, sizeof(ioeb->eh));
	if (! b)
	{
		SFQ_FAIL(EA_SEEKSETIO, "seek_set_write_(eh)");
	}

/* */
	if (ioeb->eh.payload_size)
	{
		assert(ioeb->payload);

	/* w: payload */
		iosize = fwrite(ioeb->payload, ioeb->eh.payload_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILE, "FILE-WRITE(payload)");
		}
	}

/* null term strings */

	WRITEELM_NTSTR(eworkdir);
	WRITEELM_NTSTR(execpath);
	WRITEELM_NTSTR(execargs);
	WRITEELM_NTSTR(metatext);
	WRITEELM_NTSTR(soutpath);
	WRITEELM_NTSTR(serrpath);

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	return SFQ_LIB_IS_SUCCESS();
}

#define READELM_NTSTR(key_) \
	\
	if (ioeb->eh.key_ ## _size) \
	{ \
		key_ = malloc(ioeb->eh.key_ ## _size); \
		if (! key_) \
		{ \
			SFQ_FAIL(ES_MEMORY, "malloc"); \
		} \
		if (fread(key_, ioeb->eh.key_ ## _size, 1, qo->fp) != 1) \
		{ \
			SFQ_FAIL(ES_FILE, "fread"); \
		} \
	}

/*
printf("[%.*s] %zu\n", (int)ioeb->eh.key_ ## _size, key_, (size_t)ioeb->eh.key_ ## _size); \
*/

sfq_bool sfq_readelm_alloc(struct sfq_queue_object* qo, off_t seek_pos, struct sfq_ioelm_buff* ioeb)
{
SFQ_LIB_ENTER

	sfq_bool b = SFQ_false;
	size_t iosize = 0;
	size_t eh_size = 0;

	char* eworkdir = NULL;
	char* execpath = NULL;
	char* execargs = NULL;
	char* metatext = NULL;
	sfq_byte* payload = NULL;
	char* soutpath = NULL;
	char* serrpath = NULL;

/* */
	eh_size = sizeof(ioeb->eh);

	if (! qo)
	{
		SFQ_FAIL(EA_FUNCARG, "qo");
	}

	sfq_init_ioeb(ioeb);

/* read element */

	/* r: header */
	b = seek_set_read_(qo->fp, seek_pos, &ioeb->eh, eh_size);
	if (! b)
	{
		SFQ_FAIL(EA_SEEKSETIO, "seek_set_read_(eh)");
	}

	if (ioeb->eh.eh_size != eh_size)
	{
		SFQ_FAIL(EA_ILLEGALVER, "ioeb->eh.eh_size != eh_size");
	}

/* */
	if (ioeb->eh.payload_size)
	{
	/* r: payload */
		payload = malloc(ioeb->eh.payload_size);
		if (! payload)
		{
			SFQ_FAIL(ES_MEMORY, "ALLOC(payload_size)");
		}

		iosize = fread(payload, ioeb->eh.payload_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILE, "FILE-READ(payload)");
		}
	}

/* */
	READELM_NTSTR(eworkdir);
	READELM_NTSTR(execpath);
	READELM_NTSTR(execargs);
	READELM_NTSTR(metatext);
	READELM_NTSTR(soutpath);
	READELM_NTSTR(serrpath);

	ioeb->payload = payload;
	ioeb->eworkdir = eworkdir;
	ioeb->execpath = execpath;
	ioeb->execargs = execargs;
	ioeb->metatext = metatext;
	ioeb->soutpath = soutpath;
	ioeb->serrpath = serrpath;

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		free(payload);
		free(eworkdir);
		free(execpath);
		free(execargs);
		free(metatext);
		free(soutpath);
		free(serrpath);
	}

SFQ_LIB_LEAVE

	return SFQ_LIB_IS_SUCCESS();
}

void sfq_free_ioelm_buff(struct sfq_ioelm_buff* ioeb)
{
	if (! ioeb)
	{
		return;
	}

	free((char*)ioeb->payload);
	free((char*)ioeb->eworkdir);
	free((char*)ioeb->execpath);
	free((char*)ioeb->execargs);
	free((char*)ioeb->metatext);
	free((char*)ioeb->soutpath);
	free((char*)ioeb->serrpath);

	sfq_init_ioeb(ioeb);
}


void sfq_qh_init_pos(struct sfq_q_header* p)
{
	if (! p)
	{
		return;
	}

	p->dval.elm_next_pop_pos = p->dval.elm_next_shift_pos =
	p->dval.elm_num = p->dval.elmsize_total_ = 0;

	p->dval.elm_next_push_pos = p->sval.elmseg_start_pos;
}

