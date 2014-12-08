#include "sfq-lib.h"

static struct sfq_queue_object* open_queue_(const char* querootdir, const char* quename,
	const char* file_mode)
{
SFQ_LIB_INITIALIZE

	struct sfq_queue_object* qo = NULL;

	struct sfq_open_names* om = NULL;
	sem_t* semobj = NULL;
	FILE* fp = NULL;

	int irc = 0;
	bool locked = false;

/* check argument */
	if (! file_mode)
	{
		SFQ_FAIL(EA_FUNCARG, "arg(file_mode) is null");
	}

/* create names */
	om = sfq_alloc_open_names(querootdir, quename);
	if (! om)
	{
		SFQ_FAIL(EA_CREATENAMES, "sfq_alloc_open_names");
	}

/* open semaphore */
	semobj = sem_open(om->semname, O_CREAT, 0600, 1);
	if (semobj == SEM_FAILED)
	{
		SFQ_FAIL(ES_SEMOPEN, "sem_open");
	}

/* lock */
	irc = sem_wait(semobj);
	if (irc == -1)
	{
		SFQ_FAIL(ES_SEMIO, "sem_wait");
	}
	locked = true;

/* open queue-file */
	fp = fopen(om->quefile, file_mode);
	if (! fp)
	{
		SFQ_FAIL(ES_FILEOPEN, "file open error '%s'", om->quefile);
	}

/* create response */
	qo = malloc(sizeof(*qo));
	if (! qo)
	{
		SFQ_FAIL(ES_MEMALLOC, "malloc(locked_file)");
	}

	qo->om = om;
	qo->semobj = semobj;
	qo->fp = fp;
	qo->opentime = time(NULL);

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		if (fp)
		{
			fclose(fp);
			fp = NULL;
		}

		if (semobj)
		{
			if (locked)
			{
				sem_post(semobj);
			}
			sem_close(semobj);
			semobj = NULL;
		}

		sfq_free_open_names(om);
		om = NULL;

		free(qo);
		qo = NULL;
	}

SFQ_LIB_FINALIZE

	return qo;
}

struct sfq_queue_object* sfq_open_queue_rw(const char* querootdir, const char* quename)
{
	struct sfq_queue_object* qo = open_queue_(querootdir, quename, "rb+");

	if (qo)
	{
		qo->file_openmode = (SFQ_FOM_READ | SFQ_FOM_WRITE);
	}

	return qo;
}

struct sfq_queue_object* sfq_open_queue_ro(const char* querootdir, const char* quename)
{
	struct sfq_queue_object* qo = open_queue_(querootdir, quename, "rb");

	if (qo)
	{
		qo->file_openmode = (SFQ_FOM_READ);
	}

	return qo;
}

struct sfq_queue_object* sfq_create_queue(const char* querootdir, const char* quename)
{
SFQ_LIB_INITIALIZE

	struct sfq_queue_object* qo = NULL;
	struct sfq_open_names* chk_om = NULL;

	int irc = 0;
	struct stat stbuf;

/* */
	chk_om = sfq_alloc_open_names(querootdir, quename);
	if (! chk_om)
	{
		SFQ_FAIL(EA_CREATENAMES, "sfq_alloc_open_names");
	}

/* queue-dir not exists */
	irc = stat(chk_om->querootdir, &stbuf);
	if (irc != 0)
	{
		/* already exists */
		SFQ_FAIL(EA_PATHNOTEXIST, "dir not exist '%s'", chk_om->querootdir);
	}

/* queue file, already exists */
	irc = stat(chk_om->quedir, &stbuf);
	if (irc == 0)
	{
		/* already exists */
		SFQ_FAIL(EA_EXISTQUEUE, "dir already exist '%s'", chk_om->quedir);
	}

/* delete old lock */
	irc = sem_unlink(chk_om->semname);
	if (irc == -1)
	{
		if (errno != ENOENT)
		{
			SFQ_FAIL(ES_UNLINK, "sem_unlink");
		}
	}

/* make directories */
	irc = mkdir(chk_om->quedir, 0700);
	if (irc != 0)
	{
		SFQ_FAIL(ES_MKDIR, "quedir");
	}

	irc = mkdir(chk_om->quelogdir, 0700);
	if (irc != 0)
	{
		SFQ_FAIL(ES_MKDIR, "quelogdir");
	}

	irc = mkdir(chk_om->queproclogdir, 0700);
	if (irc != 0)
	{
		SFQ_FAIL(ES_MKDIR, "queproclogdir");
	}

	irc = mkdir(chk_om->queexeclogdir, 0700);
	if (irc != 0)
	{
		SFQ_FAIL(ES_MKDIR, "queexeclogdir");
	}

	sfq_free_open_names(chk_om);
	chk_om = NULL;

/* open queue (w) */
	qo = open_queue_(querootdir, quename, "wb");
	if (! qo)
	{
		SFQ_FAIL(EA_OPENFILE, "open_queue_");
	}

	qo->file_openmode = (SFQ_FOM_WRITE);

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		sfq_free_open_names(chk_om);
		chk_om = NULL;

		sfq_close_queue(qo);
		qo = NULL;
	}

SFQ_LIB_FINALIZE

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
	}

	if (qo->semobj)
	{
		sem_post(qo->semobj);
		sem_close(qo->semobj);
	}

	sfq_free_open_names(qo->om);

	free(qo);
}

bool sfq_writeqfh(struct sfq_queue_object* qo, struct sfq_file_header* qfh,
	const struct sfq_process_info* procs, const char* lastoper)
{
SFQ_LIB_INITIALIZE

	bool b = false;

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

	b = sfq_seek_set_and_write(qo->fp, 0, qfh, sizeof(*qfh));
	if (! b)
	{
		SFQ_FAIL(EA_WRITEQFH, "sfq_seek_set_and_write");
	}

	if (procs)
	{
		size_t iosize = 0;
		size_t procs_size = 0;
		size_t pi_size = 0;

		assert(qfh->qh.sval.procs_num > 0);

		pi_size = sizeof(struct sfq_process_info);
		procs_size = (pi_size * qfh->qh.sval.procs_num);

		iosize = fwrite(procs, procs_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-WRITE(procs)");
		}
	}

SFQ_LIB_CHECKPOINT

SFQ_LIB_FINALIZE

	return SFQ_LIB_IS_SUCCESS();
}

bool sfq_readqfh(struct sfq_queue_object* qo, struct sfq_file_header* qfh,
	struct sfq_process_info** procs_ptr)
{
SFQ_LIB_INITIALIZE

	struct sfq_process_info* procs = NULL;
	bool b = false;

/* */
	if (! qo)
	{
		SFQ_FAIL(EA_FUNCARG, "qo");
	}

	bzero(qfh, sizeof(*qfh));

/* read file-header */
	b = sfq_seek_set_and_read(qo->fp, 0, qfh, sizeof(*qfh));
	if (! b)
	{
		SFQ_FAIL(EA_SEEKSETIO, "sfq_seek_set_and_read(qfh)");
	}

/* check magic string */
	if (strncmp(qfh->magicstr, SFQ_MAGICSTR, strlen(SFQ_MAGICSTR)) != 0)
	{
		SFQ_FAIL(EA_ILLEGALVER, "magicstr");
	}

/* check data version */
	if (qfh->qfh_size != sizeof(*qfh))
	{
		SFQ_FAIL(EA_ILLEGALVER, "qfh_size not match");
	}

/* read process-table */
	if (procs_ptr)
	{
		if (qfh->qh.sval.procs_num)
		{
			size_t iosize = 0;
			size_t procs_size = 0;

			procs_size = qfh->qh.sval.procs_num * sizeof(struct sfq_process_info);
			procs = malloc(procs_size);

			if (! procs)
			{
				SFQ_FAIL(ES_MEMALLOC, "ALLOC(procs)");
			}

			iosize = fread(procs, procs_size, 1, qo->fp);
			if (iosize != 1)
			{
				SFQ_FAIL(ES_FILEIO, "FILE-READ(procs)");
			}

#ifdef SFQ_DEBUG_BUILD
/*
			sfq_print_procs(procs, qfh->qh.sval.procs_num);
*/
#endif

			(*procs_ptr) = procs;
		}
	}

	if (qo->file_openmode & SFQ_FOM_WRITE)
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

SFQ_LIB_FINALIZE

	return SFQ_LIB_IS_SUCCESS();
}

/*
 * element
 *    - header
 *    - payload  (eh.payload_size  >= 0)
 *    - execpath (eh.execpath_size >= 0) ... nullterm string
 *    - execargs (eh.execargs_size >= 0) ... nullterm string
 *    - metatext (eh.metatext_size >= 0) ... nullterm string
 *    - soutpath (eh.soutpath_size >= 0) ... nullterm string
 *    - serrpath (eh.serrpath_size >= 0) ... nullterm string
 *
 */
bool sfq_writeelm(struct sfq_queue_object* qo, off_t seek_pos, const struct sfq_ioelm_buff* ioeb)
{
SFQ_LIB_INITIALIZE

	bool b = false;
	size_t iosize = 0;

	if (! qo)
	{
		SFQ_FAIL(EA_FUNCARG, "qo");
	}

	b = sfq_seek_set_and_write(qo->fp, seek_pos, &ioeb->eh, sizeof(ioeb->eh));
	if (! b)
	{
		SFQ_FAIL(EA_SEEKSETIO, "sfq_seek_set_and_write");
	}

	if (ioeb->eh.payload_size)
	{
		assert(ioeb->payload);

	/* w: payload */
		iosize = fwrite(ioeb->payload, ioeb->eh.payload_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-WRITE(payload)");
		}
	}

	if (ioeb->eh.execpath_size)
	{
		assert(ioeb->execpath);

	/* w: execpath */
		iosize = fwrite(ioeb->execpath, ioeb->eh.execpath_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-WRITE(execpath)");
		}
	}

	if (ioeb->eh.execargs_size)
	{
		assert(ioeb->execargs);

	/* w: execargs */
		iosize = fwrite(ioeb->execargs, ioeb->eh.execargs_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-WRITE(execargs)");
		}
	}

	if (ioeb->eh.metatext_size)
	{
		assert(ioeb->metatext);

	/* w: metatext */
		iosize = fwrite(ioeb->metatext, ioeb->eh.metatext_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-WRITE(metatext)");
		}
	}

	if (ioeb->eh.soutpath_size)
	{
		assert(ioeb->soutpath);

	/* w: soutpath */
		iosize = fwrite(ioeb->soutpath, ioeb->eh.soutpath_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-WRITE(soutpath)");
		}
	}

	if (ioeb->eh.serrpath_size)
	{
		assert(ioeb->serrpath);

	/* w: serrpath */
		iosize = fwrite(ioeb->serrpath, ioeb->eh.serrpath_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-WRITE(serrpath)");
		}
	}

SFQ_LIB_CHECKPOINT

SFQ_LIB_FINALIZE

	return SFQ_LIB_IS_SUCCESS();
}

bool sfq_readelm(struct sfq_queue_object* qo, off_t seek_pos, struct sfq_ioelm_buff* ioeb)
{
SFQ_LIB_INITIALIZE

	bool b = false;
	size_t iosize = 0;
	size_t eh_size = 0;

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

	bzero(ioeb, sizeof(*ioeb));

/* read element */

	/* r: header */
	b = sfq_seek_set_and_read(qo->fp, seek_pos, &ioeb->eh, eh_size);
	if (! b)
	{
		SFQ_FAIL(EA_SEEKSETIO, "sfq_seek_set_and_read(eh)");
	}

	if (ioeb->eh.eh_size != eh_size)
	{
		SFQ_FAIL(EA_ILLEGALVER, "ioeb->eh.eh_size != eh_size");
	}

	if (ioeb->eh.payload_size)
	{
	/* r: payload */
		payload = malloc(ioeb->eh.payload_size);
		if (! payload)
		{
			SFQ_FAIL(ES_MEMALLOC, "ALLOC(payload_size)");
		}

		iosize = fread(payload, ioeb->eh.payload_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-READ(payload)");
		}
	}

	if (ioeb->eh.execpath_size)
	{
	/* r: execpath */
		execpath = malloc(ioeb->eh.execpath_size);
		if (! execpath)
		{
			SFQ_FAIL(ES_MEMALLOC, "ALLOC(execpath)");
		}

		iosize = fread(execpath, ioeb->eh.execpath_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-READ(execpath)");
		}
	}

	if (ioeb->eh.execargs_size)
	{
	/* w: execargs */
		execargs = malloc(ioeb->eh.execargs_size);
		if (! execargs)
		{
			SFQ_FAIL(ES_MEMALLOC, "ALLOC(execargs)");
		}

		iosize = fread(execargs, ioeb->eh.execargs_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-READ(execargs)");
		}
	}

	if (ioeb->eh.metatext_size)
	{
	/* w: metatext */
		metatext = malloc(ioeb->eh.metatext_size);
		if (! metatext)
		{
			SFQ_FAIL(ES_MEMALLOC, "ALLOC(metatext)");
		}

		iosize = fread(metatext, ioeb->eh.metatext_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-READ(metatext)");
		}
	}

	if (ioeb->eh.soutpath_size)
	{
	/* r: soutpath */
		soutpath = malloc(ioeb->eh.soutpath_size);
		if (! soutpath)
		{
			SFQ_FAIL(ES_MEMALLOC, "ALLOC(soutpath)");
		}

		iosize = fread(soutpath, ioeb->eh.soutpath_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-READ(soutpath)");
		}
	}

	if (ioeb->eh.serrpath_size)
	{
	/* r: serrpath */
		serrpath = malloc(ioeb->eh.serrpath_size);
		if (! serrpath)
		{
			SFQ_FAIL(ES_MEMALLOC, "ALLOC(serrpath)");
		}

		iosize = fread(serrpath, ioeb->eh.serrpath_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-READ(serrpath)");
		}
	}

	ioeb->payload = payload;
	ioeb->execpath = execpath;
	ioeb->execargs = execargs;
	ioeb->metatext = metatext;
	ioeb->soutpath = soutpath;
	ioeb->serrpath = serrpath;

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		free(payload);
		free(execpath);
		free(execargs);
		free(metatext);
		free(soutpath);
		free(serrpath);
	}

SFQ_LIB_FINALIZE

	return SFQ_LIB_IS_SUCCESS();
}

void sfq_free_ioelm_buff(struct sfq_ioelm_buff* ioeb)
{
	if (! ioeb)
	{
		return;
	}

	free(ioeb->payload);
	free(ioeb->execpath);
	free(ioeb->execargs);
	free(ioeb->metatext);
	free(ioeb->soutpath);
	free(ioeb->serrpath);

	bzero(ioeb, sizeof(*ioeb));
}

bool sfq_seek_set_and_read(FILE* fp, off_t set_pos, void* mem, size_t mem_size)
{
	off_t orc = 0;
	size_t iosize = 0;

	if (! fp)
	{
		return false;
	}

	if (! mem)
	{
		return false;
	}

	if (mem_size == 0)
	{
		return false;
	}

	orc = fseeko(fp, set_pos, SEEK_SET);
	if (orc == -1)
	{
		return false;
	}

	iosize = fread(mem, mem_size, 1, fp);
	if (iosize != 1)
	{
		return false;
	}

	return true;
}

bool sfq_seek_set_and_write(FILE* fp, off_t set_pos, const void* mem, size_t mem_size)
{
	off_t orc = 0;
	size_t iosize = 0;

	if (! fp)
	{
		return false;
	}

	if (! mem)
	{
		return false;
	}

	if (mem_size == 0)
	{
		return false;
	}

	orc = fseeko(fp, set_pos, SEEK_SET);
	if (orc == -1)
	{
		return false;
	}

	iosize = fwrite(mem, mem_size, 1, fp);
	if (iosize != 1)
	{
		return false;
	}

	return true;
}

void sfq_qh_init_pos(struct sfq_q_header* p)
{
	if (! p)
	{
		return;
	}

	p->dval.elm_last_push_pos = p->dval.elm_next_shift_pos = p->dval.elm_num = 0;
	p->dval.elm_new_push_pos = p->sval.elmseg_start_pos;
}

