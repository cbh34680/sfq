#include "sfq-lib.h"

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
		SFQ_FAIL(EA_PATHNOTEXIST, "queue-dir not exists");
	}

/* queue file, already exists */
	irc = stat(chk_om->quedir, &stbuf);
	if (irc == 0)
	{
		/* already exists */
		SFQ_FAIL(EA_EXISTQUEUE, "queue-directory exists");
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
	qo = sfq_open_queue(querootdir, quename, "wb");
	if (! qo)
	{
		SFQ_FAIL(EA_OPENFILE, "sfq_open_queue");
	}

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

struct sfq_queue_object* sfq_open_queue(const char* querootdir, const char* quename, const char* file_mode)
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

#ifdef SFQ_DEBUG_BUILD
/*
	fprintf(stderr, "QUEROOTDIR=[%s]\n", qo->om->querootdir);
	fprintf(stderr, "QUENAME=[%s]\n", qo->om->quename);
	fprintf(stderr, "QUEDIR=[%s]\n", qo->om->quedir);
	fprintf(stderr, "QUEFILE=[%s]\n", qo->om->quefile);
	fprintf(stderr, "QUELOGDIR=[%s]\n", qo->om->quelogdir);
	fprintf(stderr, "QUEPROCLOGDIR=[%s]\n", qo->om->queproclogdir);
	fprintf(stderr, "QUEEXECLOGDIR=[%s]\n", qo->om->queexeclogdir);
	fprintf(stderr, "SEMAPHORE=[%s]\n", qo->om->semname);
	fprintf(stderr, "OPENTIME=[%zu]\n", qo->opentime);
	fprintf(stderr, "\n");
*/
#endif

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

void sfq_close_queue(struct sfq_queue_object* qo)
{
	if (qo)
	{
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
		free((void*)qo);
	}
}

bool sfq_readqfh(FILE* fp, struct sfq_file_header* qfh, struct sfq_process_info** pprocs)
{
SFQ_LIB_INITIALIZE

	struct sfq_process_info* procs = NULL;
	size_t procs_size = 0;

	size_t iosize = 0;

/* read file-header */
	iosize = fread(qfh, sizeof(*qfh), 1, fp);
	if (iosize != 1)
	{
		SFQ_FAIL(ES_FILEIO, "fread(qfh)");
	}

/* check magic string */
	if (strncmp(qfh->magicstr, SFQ_MAGICSTR, strlen(SFQ_MAGICSTR)) != 0)
	{
		SFQ_FAIL(EA_ILLEGALVER, "magicstr");
	}

/* check data version */
	if (qfh->qfh_size != sizeof(*qfh))
	{
		SFQ_FAIL(EA_ILLEGALVER, "qfh_size");
	}

#ifdef SFQ_DEBUG_BUILD
	sfq_print_qf_header(qfh);
#endif

/* read process-table */
	if (pprocs)
	{
		if (qfh->qh.sval.max_proc_num)
		{
			procs_size = qfh->qh.sval.max_proc_num * sizeof(struct sfq_process_info);
			procs = malloc(procs_size);

			if (! procs)
			{
				SFQ_FAIL(ES_MEMALLOC, "ALLOC(procs)");
			}

			iosize = fread(procs, procs_size, 1, fp);
			if (iosize != 1)
			{
				SFQ_FAIL(ES_FILEIO, "FILE-READ(procs)");
			}

#ifdef SFQ_DEBUG_BUILD
			sfq_print_procs(procs, qfh->qh.sval.max_proc_num);
#endif

			(*pprocs) = procs;
		}
	}

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		free(procs);
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
 *    - metadata (eh.metadata_size >= 0) ... nullterm string
 *
 */
bool sfq_writeelm(FILE* fp, off_t seek_pos, struct sfq_ioelm_buff* ioeb)
{
SFQ_LIB_INITIALIZE

	bool b = false;
	size_t iosize = 0;

	b = sfq_seek_set_and_write(fp, seek_pos, &ioeb->eh, sizeof(ioeb->eh));
	if (! b)
	{
		SFQ_FAIL(EA_SEEKSETIO, "sfq_seek_set_and_write");
	}

	if (ioeb->eh.payload_size)
	{
		assert(ioeb->payload);

	/* w: payload */
		iosize = fwrite(ioeb->payload, ioeb->eh.payload_size, 1, fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-WRITE(payload)");
		}
	}

	if (ioeb->eh.execpath_size)
	{
		assert(ioeb->execpath);

	/* w: execpath */
		iosize = fwrite(ioeb->execpath, ioeb->eh.execpath_size, 1, fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-WRITE(execpath)");
		}
	}

	if (ioeb->eh.execargs_size)
	{
		assert(ioeb->execargs);

	/* w: execargs */
		iosize = fwrite(ioeb->execargs, ioeb->eh.execargs_size, 1, fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-WRITE(execargs)");
		}
	}

	if (ioeb->eh.metadata_size)
	{
		assert(ioeb->metadata);

	/* w: metadata */
		iosize = fwrite(ioeb->metadata, ioeb->eh.metadata_size, 1, fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-WRITE(metadata)");
		}
	}

SFQ_LIB_CHECKPOINT

SFQ_LIB_FINALIZE

	return SFQ_LIB_IS_SUCCESS();
}

bool sfq_readelm(FILE* fp, off_t seek_pos, struct sfq_ioelm_buff* ioeb)
{
SFQ_LIB_INITIALIZE

	bool b = false;
	size_t iosize = 0;
	size_t eh_size = 0;

	char* execpath = NULL;
	char* execargs = NULL;
	char* metadata = NULL;
	sfq_byte* payload = NULL;

/* */
	eh_size = sizeof(ioeb->eh);

	bzero(ioeb, sizeof(*ioeb));

/* read element */

	/* r: header */
	b = sfq_seek_set_and_read(fp, seek_pos, &ioeb->eh, eh_size);
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

		iosize = fread(payload, ioeb->eh.payload_size, 1, fp);
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

		iosize = fread(execpath, ioeb->eh.execpath_size, 1, fp);
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

		iosize = fread(execargs, ioeb->eh.execargs_size, 1, fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-READ(execargs)");
		}
	}

	if (ioeb->eh.metadata_size)
	{
	/* w: metadata */
		metadata = malloc(ioeb->eh.metadata_size);
		if (! metadata)
		{
			SFQ_FAIL(ES_MEMALLOC, "ALLOC(metadata)");
		}

		iosize = fread(metadata, ioeb->eh.metadata_size, 1, fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-READ(metadata)");
		}
	}

	ioeb->execpath = execpath;
	ioeb->execargs = execargs;
	ioeb->metadata = metadata;
	ioeb->payload = payload;

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		free(execpath);
		free(execargs);
		free(metadata);
		free(payload);
	}

SFQ_LIB_FINALIZE

	return SFQ_LIB_IS_SUCCESS();
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

bool sfq_seek_set_and_write(FILE* fp, off_t set_pos, void* mem, size_t mem_size)
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
	if (p)
	{
		p->dval.elm_last_push_pos = p->dval.elm_next_shift_pos = p->dval.elm_num = 0;
		p->dval.elm_new_push_pos = p->sval.elmseg_start_pos;
	}
}

void sfq_free_ioelm_buff(struct sfq_ioelm_buff* ioeb)
{
	if (ioeb)
	{
		free(ioeb->execpath);
		free(ioeb->execargs);
		free(ioeb->metadata);
		free(ioeb->payload);
	}
}

