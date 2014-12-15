#include "sfq-lib.h"

#ifndef __GNUC__
char* sfq_safe_strcpy(char* dst, const char* org)
{
	if (dst && org)
	{
		return strcpy(dst, org);
	}

	return NULL;
}
#endif

size_t sfq_payload_len(const struct sfq_value* val)
{
	size_t ret = 0;

	if (val)
	{
		if (val->payload)
		{
			ret = val->payload_size;

			if (val->payload_type & SFQ_PLT_NULLTERM)
			{
				ret--;
			}
		}
	}

	return ret;
}

int sfq_reserve_proc(struct sfq_process_info* procs, ushort procs_num)
{
	int i = 0;

	for (i=0; i<procs_num; i++)
	{
		if (procs[i].procstate)
		{
			continue;
		}

		procs[i].ppid = getpid();
		procs[i].procstate = SFQ_PIS_WAITFOR;
		procs[i].updtime = time(NULL);

		return i;
	}

	return -1;
}

int sfq_get_questate(const char* querootdir, const char* quename, questate_t* questate_ptr)
{
SFQ_LIB_ENTER

	struct sfq_queue_object* qo = NULL;

	struct sfq_file_header qfh;
	bool b = false;

/* initialize */
	bzero(&qfh, sizeof(qfh));

	if (! questate_ptr)
	{
		SFQ_FAIL(EA_FUNCARG, "questate_ptr is null");
	}

	qo = sfq_open_queue_ro(querootdir, quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENFILE, "sfq_open_queue");
	}

	b = sfq_readqfh(qo, &qfh, NULL);
	if (! b)
	{
		SFQ_FAIL(EA_READQFH, "sfq_readqfh");
	}

	(*questate_ptr) = qfh.qh.dval.questate;

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	sfq_close_queue(qo);
	qo = NULL;

	return SFQ_LIB_RC();
}

int sfq_set_questate(const char* querootdir, const char* quename, questate_t questate)
{
SFQ_LIB_ENTER

	struct sfq_open_names* om = NULL;
	struct sfq_queue_object* qo = NULL;
	struct sfq_process_info* procs = NULL;

	int irc = -1;
	int slotno = -1;
	bool b = false;

	bool forceLeakQueue = false;

	struct sfq_file_header qfh;

/* initialize */
	bzero(&qfh, sizeof(qfh));

/* create names */
	if (questate & SFQ_QST_DEV_SEMUNLOCK_ON)
	{
/*
!! デバッグ用 !!

強制的にセマフォのロックを解除する
*/
		om = sfq_alloc_open_names(querootdir, quename);
		if (! om)
		{
			SFQ_FAIL(EA_CREATENAMES, "sfq_alloc_open_names");
		}

		irc = sem_unlink(om->semname);
		if (irc == -1)
		{
			if (errno != ENOENT)
			{
				SFQ_FAIL(ES_UNLINK,
					"delete semaphore fault, check permission (e.g. /dev/shm%s)",
					om->semname);
			}
		}

		SFQ_FAIL(DEV_SEMUNLOCK, "success unlock semaphore [%s] (for develop)\n", om->semname);
	}

/* open queue */
	qo = sfq_open_queue_rw(querootdir, quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENFILE, "sfq_open_queue");
	}

/* */
	if (questate & SFQ_QST_DEV_SEMLOCK_ON)
	{
/*
!! デバッグ用 !!

セマフォをロッセしたままにする
--> メモリリークは発生する
*/
		forceLeakQueue = true;

		SFQ_FAIL(DEV_SEMLOCK, "success lock semaphore [%s] (for develop)\n", qo->om->semname);
	}

/* read queue header */
	b = sfq_readqfh(qo, &qfh, &procs);
	if (! b)
	{
		SFQ_FAIL(EA_READQFH, "sfq_readqfh");
	}

/* update state */
	if (qfh.qh.dval.questate == questate)
	{
/*
変更前後が同じ値なら更新の必要はない
*/
		SFQ_FAIL(W_NOCHANGE_STATE, "there is no change in the state");
	}

	if (questate & SFQ_QST_EXEC_ON)
	{
/*
exec が OFF から ON に変わった
*/
		if (qfh.qh.dval.elm_num)
		{
/*
キューに要素が存在する
*/
			if (procs)
			{
/*
プロセステーブルが存在するので、実行予約を試みる
*/
				slotno = sfq_reserve_proc(procs, qfh.qh.sval.procs_num);
			}
		}
	}

	if (slotno == -1)
	{
/*
プロセステーブルが更新されていないので開放
*/
		free(procs);
		procs = NULL;
	}

	qfh.qh.dval.questate = questate;

	b = sfq_writeqfh(qo, &qfh, procs, "SET");
	if (! b)
	{
		SFQ_FAIL(EA_WRITEQFH, "sfq_writeqfh");
	}

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	sfq_free_open_names(om);
	om = NULL;

	free(procs);
	procs = NULL;

	if (forceLeakQueue)
	{
		/* for Debug */
	}
	else
	{
		sfq_close_queue(qo);
		qo = NULL;
	}

	if (slotno != -1)
	{
		sfq_go_exec(querootdir, quename, (ushort)slotno, questate);
	}

	return SFQ_LIB_RC();
}

bool sfq_copy_val2ioeb(const struct sfq_value* val, struct sfq_ioelm_buff* ioeb)
{
SFQ_LIB_ENTER

	size_t eh_size = 0;
	size_t add_all = 0;
	sfq_uchar elmmargin_ = 0;

	if (! ioeb)
	{
		SFQ_FAIL(EA_FUNCARG, "ioeb");
	}

	if (! val)
	{
		SFQ_FAIL(EA_FUNCARG, "val");
	}

/* initialize */
	eh_size = sizeof(ioeb->eh);

	bzero(ioeb, sizeof(*ioeb));

/* */
	ioeb->eh.eh_size = eh_size;
	ioeb->eh.id = val->id;
	ioeb->eh.pushtime = val->pushtime;

	uuid_copy(ioeb->eh.uuid, val->uuid);

	if (val->execpath)
	{
		size_t execpath_len = strlen(val->execpath);
		if (execpath_len)
		{
			size_t execpath_size = execpath_len + 1;

			if (execpath_size >= USHRT_MAX)
			{
				SFQ_FAIL(EA_OVERLIMIT, "execpath_size");
			}
			if (execpath_size >= PATH_MAX)
			{
				SFQ_FAIL(EA_OVERLIMIT, "execpath_size");
			}

			ioeb->execpath = val->execpath;
			ioeb->eh.execpath_size = (ushort)execpath_size;
		}
	}

	if (val->execargs)
	{
		size_t execargs_len = strlen(val->execargs);
		if (execargs_len)
		{
			long sysmax = sysconf(_SC_ARG_MAX);
			size_t execargs_size = execargs_len + 1;

			if (execargs_size >= UINT_MAX)
			{
				SFQ_FAIL(EA_OVERLIMIT, "execargs_size");
			}

			if (sysmax > 0)
			{
				if (execargs_size >= (size_t)sysmax)
				{
					SFQ_FAIL(EA_OVERLIMIT, "execargs_size");
				}
			}

			ioeb->execargs = val->execargs;
			ioeb->eh.execargs_size = (uint)execargs_size;
		}
	}

	if (val->metatext)
	{
		size_t metatext_len = strlen(val->metatext);
		if (metatext_len)
		{
			size_t metatext_size = metatext_len + 1;

			if (metatext_size >= USHRT_MAX)
			{
				SFQ_FAIL(EA_OVERLIMIT, "metatext_size");
			}

			ioeb->metatext = val->metatext;
			ioeb->eh.metatext_size = (ushort)metatext_size;
		}
	}

	if (val->payload_size)
	{
		assert(val->payload_type);
		assert(val->payload);

		ioeb->eh.payload_size = val->payload_size;
		ioeb->eh.payload_type = val->payload_type;
		ioeb->payload = val->payload;
	}

	if (val->soutpath)
	{
		size_t soutpath_len = strlen(val->soutpath);
		if (soutpath_len)
		{
			size_t soutpath_size = soutpath_len + 1;

			if (soutpath_size >= USHRT_MAX)
			{
				SFQ_FAIL(EA_OVERLIMIT, "soutpath_size");
			}
			if (soutpath_size >= PATH_MAX)
			{
				SFQ_FAIL(EA_OVERLIMIT, "soutpath_size");
			}

			ioeb->soutpath = val->soutpath;
			ioeb->eh.soutpath_size = (ushort)soutpath_size;
		}
	}

	if (val->serrpath)
	{
		size_t serrpath_len = strlen(val->serrpath);
		if (serrpath_len)
		{
			size_t serrpath_size = serrpath_len + 1;

			if (serrpath_size >= USHRT_MAX)
			{
				SFQ_FAIL(EA_OVERLIMIT, "serrpath_size");
			}
			if (serrpath_size >= PATH_MAX)
			{
				SFQ_FAIL(EA_OVERLIMIT, "serrpath_size");
			}

			ioeb->serrpath = val->serrpath;
			ioeb->eh.serrpath_size = (ushort)serrpath_size;
		}
	}

/* for debug */
	add_all =
	(
		eh_size +
		ioeb->eh.execpath_size +
		ioeb->eh.execargs_size +
		ioeb->eh.metatext_size +
		ioeb->eh.payload_size  +
		ioeb->eh.soutpath_size +
		ioeb->eh.serrpath_size
	);

	elmmargin_ = SFQ_ALIGN_MARGIN(add_all);

	ioeb->eh.elmsize_ = add_all + elmmargin_;
	ioeb->eh.elmmargin_ = elmmargin_;

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	return SFQ_LIB_IS_SUCCESS();
}

bool sfq_copy_ioeb2val(const struct sfq_ioelm_buff* ioeb, struct sfq_value* val)
{
	if (! ioeb)
	{
		return false;
	}

	if (! val)
	{
		return false;
	}

	bzero(val, sizeof(*val));

	val->id = ioeb->eh.id;
	val->pushtime = ioeb->eh.pushtime;

	uuid_copy(val->uuid, ioeb->eh.uuid);

	if (ioeb->eh.execpath_size)
	{
		assert(ioeb->execpath);
		val->execpath = ioeb->execpath;
	}

	if (ioeb->eh.execargs_size)
	{
		assert(ioeb->execargs);
		val->execargs = ioeb->execargs;
	}

	if (ioeb->eh.metatext_size)
	{
		assert(ioeb->metatext);
		val->metatext = ioeb->metatext;
	}

	if (ioeb->eh.soutpath_size)
	{
		assert(ioeb->soutpath);
		val->soutpath = ioeb->soutpath;
	}

	if (ioeb->eh.serrpath_size)
	{
		assert(ioeb->serrpath);
		val->serrpath = ioeb->serrpath;
	}

	if (ioeb->eh.payload_size)
	{
		assert(ioeb->eh.payload_type);
		assert(ioeb->payload);

		val->payload_size = ioeb->eh.payload_size;
		val->payload_type = ioeb->eh.payload_type;
		val->payload = ioeb->payload;
	}

	return true;
}

void sfq_free_value(struct sfq_value* val)
{
	if (! val)
	{
		return;
	}

	free((char*)val->execpath);
	free((char*)val->execargs);
	free((char*)val->metatext);
	free((char*)val->payload);
	free((char*)val->soutpath);
	free((char*)val->serrpath);

	bzero(val, sizeof(*val));
}

int sfq_alloc_print_value(const struct sfq_value* val, struct sfq_value* dst)
{
SFQ_LIB_ENTER

	const char* NA = "N/A";

	char* execpath = NULL;
	char* execargs = NULL;
	char* metatext = NULL;
	char* payload = NULL;
	char* soutpath = NULL;
	char* serrpath = NULL;

/* */
	if (! val)
	{
		SFQ_FAIL(EA_FUNCARG, "val");
	}

	if (! dst)
	{
		SFQ_FAIL(EA_FUNCARG, "dst");
	}

	bzero(dst, sizeof(*dst));

	execpath = strdup(val->execpath ? val->execpath : NA);
	if (! execpath)
	{
		SFQ_FAIL(ES_STRDUP, "execpath");
	}

	execargs = strdup(val->execargs ? val->execargs : NA);
	if (! execargs)
	{
		SFQ_FAIL(ES_STRDUP, "execargs");
	}

	metatext = strdup(val->metatext ? val->metatext : NA);
	if (! metatext)
	{
		SFQ_FAIL(ES_STRDUP, "metatext");
	}

	soutpath = strdup(val->soutpath ? val->soutpath : NA);
	if (! soutpath)
	{
		SFQ_FAIL(ES_STRDUP, "soutpath");
	}

	serrpath = strdup(val->serrpath ? val->serrpath : NA);
	if (! serrpath)
	{
		SFQ_FAIL(ES_STRDUP, "serrpath");
	}

	if (val->payload)
	{
		if (val->payload_type & SFQ_PLT_CHARARRAY)
		{
			if (val->payload_type & SFQ_PLT_NULLTERM)
			{
				payload = strdup((char*)val->payload);
			}
			else
			{
				payload = malloc(val->payload_size + 1);
				if (payload)
				{
					memcpy(payload, (char*)val->payload, val->payload_size);
					payload[val->payload_size] = '\0';
				}
			}
		}
		else
		{
			payload = strdup("BINARY");
		}
	}
	else
	{
		payload = strdup(NA);
	}

	if (! payload)
	{
		SFQ_FAIL(ES_STRDUP, "payload");
	}

	dst->id = val->id;
	dst->pushtime = val->pushtime;
	uuid_copy(dst->uuid, val->uuid);

	dst->payload_type = val->payload_type;
	dst->payload_size = val->payload_size;
	dst->payload = (sfq_byte*)payload;

	dst->execpath = execpath;
	dst->execargs = execargs;
	dst->metatext = metatext;
	dst->soutpath = soutpath;
	dst->serrpath = serrpath;

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		free(execpath);
		free(execargs);
		free(metatext);
		free(payload);
		free(soutpath);
		free(serrpath);
	}

SFQ_LIB_LEAVE

	return SFQ_LIB_RC();
}

struct sfq_open_names* sfq_alloc_open_names(const char* querootdir, const char* quename)
{
SFQ_LIB_ENTER

	struct sfq_open_names* om = NULL;

	char* om_querootdir = NULL;
	size_t om_querootdir_len = 0;

	size_t quename_len = 0;
	char* om_quename = NULL;

	char* quedir = NULL;
	size_t quedir_len = 0;
	size_t quedir_size = 0;

	char* quefile = NULL;
	size_t quefile_len = 0;
	size_t quefile_size = 0;

	char* quelogdir = NULL;
	size_t quelogdir_len = 0;
	size_t quelogdir_size = 0;

	char* queproclogdir = NULL;
	size_t queproclogdir_size = 0;

	char* queexeclogdir = NULL;
	size_t queexeclogdir_size = 0;

	char* semname = NULL;
	char* cpos = NULL;

/* */
	if (querootdir)
	{
/*
空文字列("") のときは NULL と同じ動作にする
*/
		if (strlen(querootdir) == 0)
		{
			querootdir = NULL;
		}
	}
	if (quename)
	{
/*
空文字列("") のときは NULL と同じ動作にする
*/
		if (strlen(quename) == 0)
		{
			quename = NULL;
		}
	}

/* */
	if (! querootdir)
	{
		querootdir = getenv("SFQ_QUEUE_DIR");
		if (! querootdir)
		{
			querootdir = SFQ_DEFAULT_QUEUE_DIR;
		}
	}

	if (! quename)
	{
		quename = getenv("SFQ_QUEUE_NAME");
		if (! quename)
		{
			quename = SFQ_DEFAULT_QUEUE_NAME;
		}
	}

	quename_len = strlen(quename);
	if (quename_len == 0)
	{
		SFQ_FAIL(EA_FUNCARG, "quename_len == 0");
	}

/* save rootdir realpath */
	om_querootdir = realpath(querootdir, NULL);
	if (! om_querootdir)
	{
		SFQ_FAIL(ES_REALPATH, "om_realpath");
	}

/*
querootdir は相対パスの可能性があるので、以降では参照せず
om_querootdir を使うこと
*/

	om_querootdir_len = strlen(om_querootdir);
	if (om_querootdir_len == 0)
	{
		SFQ_FAIL(EA_FUNCARG, "om_querootdir_len == 0");
	}

/* quedir */
	/* "ROOT/NAME\0" */
	quedir_len = om_querootdir_len + 1 + quename_len;
	quedir_size = quedir_len + 1;
	if (quedir_size >= PATH_MAX)
	{
		SFQ_FAIL(EA_OVERLIMIT, "quedir_size");
	}

	quedir = malloc(quedir_size);
	if (! quedir)
	{
		SFQ_FAIL(ES_MEMALLOC, "quedir");
	}
	snprintf(quedir, quedir_size, "%s/%s", om_querootdir, quename);

/* quefile */
	/* "QDIR/filename\0" */
	quefile_len = quedir_len + 1 + strlen(SFQ_QUEUE_FILENAME);
	quefile_size = quefile_len + 1;
	if (quefile_size >= PATH_MAX)
	{
		SFQ_FAIL(EA_OVERLIMIT, "quefile_size");
	}

	quefile = malloc(quefile_size);
	if (! quefile)
	{
		SFQ_FAIL(ES_MEMALLOC, "quefile");
	}
	snprintf(quefile, quefile_size, "%s/%s", quedir, SFQ_QUEUE_FILENAME);

/* quelogdir */
	/* "QDIR/logdir\0" */
	quelogdir_len = quedir_len + 1 + strlen(SFQ_QUEUE_LOGDIRNAME);
	quelogdir_size = quelogdir_len + 1;
	if (quelogdir_size >= PATH_MAX)
	{
		SFQ_FAIL(EA_OVERLIMIT, "quelogdir_size");
	}

	quelogdir = malloc(quelogdir_size);
	if (! quelogdir)
	{
		SFQ_FAIL(ES_MEMALLOC, "quelogdir");
	}
	snprintf(quelogdir, quelogdir_size, "%s/%s", quedir, SFQ_QUEUE_LOGDIRNAME);

/* queproclogdir */
	/* "QLDIR/proc\0 */
	queproclogdir_size = quelogdir_len + 1 + strlen(SFQ_QUEUE_PROC_LOGDIRNAME) + 1;
	if (queproclogdir_size >= PATH_MAX)
	{
		SFQ_FAIL(EA_OVERLIMIT, "queproclogdir_size");
	}

	queproclogdir = malloc(queproclogdir_size);
	if (! queproclogdir)
	{
		SFQ_FAIL(ES_MEMALLOC, "queproclogdir");
	}
	snprintf(queproclogdir, queproclogdir_size, "%s/%s", quelogdir, SFQ_QUEUE_PROC_LOGDIRNAME);

/* queexeclogdir */
	/* "QLDIR/exec\0 */
	queexeclogdir_size = quelogdir_len + 1 + strlen(SFQ_QUEUE_EXEC_LOGDIRNAME) + 1;
	if (queexeclogdir_size >= PATH_MAX)
	{
		SFQ_FAIL(EA_OVERLIMIT, "queexeclogdir_size");
	}

	queexeclogdir = malloc(queexeclogdir_size);
	if (! queexeclogdir)
	{
		SFQ_FAIL(ES_MEMALLOC, "queexeclogdir");
	}
	snprintf(queexeclogdir, queexeclogdir_size, "%s/%s", quelogdir, SFQ_QUEUE_EXEC_LOGDIRNAME);

/* semname */
	/* "/NAME\0" */
	semname = strdup(quefile);
	if (! semname)
	{
		SFQ_FAIL(ES_MEMALLOC, "semname");
	}

	if (quefile_len > 1)
	{
		cpos = &semname[1];
		while (*cpos)
		{
			if ((*cpos) == '/')
			{
				(*cpos) = '`';
			}
			cpos++;
		}
	}

/* save argument */
	om_quename = strdup(quename);
	if (! om_quename)
	{
		SFQ_FAIL(ES_STRDUP, "om_quename");
	}

/* */
	om = malloc(sizeof(*om));
	if (! om)
	{
		SFQ_FAIL(ES_MEMALLOC, "om");
	}

	om->querootdir = om_querootdir;
	om->quename = om_quename;
	om->quedir = quedir;
	om->quefile = quefile;
	om->quelogdir = quelogdir;
	om->queproclogdir = queproclogdir;
	om->queexeclogdir = queexeclogdir;
	om->semname = semname;

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		free(om_querootdir);
		om_querootdir = NULL;

		free(om_quename);
		om_quename = NULL;

		free(quedir);
		quedir = NULL;

		free(quefile);
		quefile = NULL;

		free(quelogdir);
		quelogdir = NULL;

		free(queproclogdir);
		queproclogdir = NULL;

		free(queexeclogdir);
		queexeclogdir = NULL;

		free(semname);
		semname = NULL;

		free(om);
		om = NULL;
	}

SFQ_LIB_LEAVE

	return om;
}

void sfq_free_open_names(struct sfq_open_names* om)
{
	if (! om)
	{
		return;
	}

	free((char*)om->querootdir);
	free((char*)om->quename);
	free((char*)om->quedir);
	free((char*)om->quefile);
	free((char*)om->quelogdir);
	free((char*)om->queproclogdir);
	free((char*)om->queexeclogdir);
	free((char*)om->semname);
	free(om);
}

/*
http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
*/
bool sfq_mkdir_p(const char *arg, mode_t perm)
{
	char* dir = NULL;
	char *p = NULL;
	size_t len = 0;


	dir = sfq_stradup(arg);
	if (! dir)
	{
		return false;
	}

	len = strlen(dir);
	if (dir[len - 1] == '/')
	{
		dir[len - 1] = 0;
	}

	for(p = dir+1; *p; p++)
	{
		if(*p == '/')
		{
			*p = '\0';
			if (mkdir(dir, perm) == -1)
			{
				if (errno != EEXIST)
				{
					return false;
				}
			}
			*p = '/';
		}
	}

	if (mkdir(dir, perm) == -1)
	{
		if (errno != EEXIST)
		{
			return false;
		}
	}

	return true;
}

char* sfq_alloc_concat_n(int n, ...)
{
	int i = 0;
	size_t len = 0;
	char* ret = NULL;

	va_list list;

	va_start(list, n);
	for (i=0; i<n; i++) { len += strlen(va_arg(list, char*)); }
	va_end(list);

	ret = malloc(len + 1 /* '\0' */);
	if (! ret)
	{
		return NULL;
	}
	ret[0] = '\0';

	va_start(list, n);
	for (i=0; i<n; i++) { strcat(ret, va_arg(list, char*)); }
	va_end(list);

	return ret;
}

