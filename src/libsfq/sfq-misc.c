#include "sfq-lib.h"

int sfq_push_str(const char* querootdir, const char* quename, const char* execpath, const char* execargs, const char* metadata, const char* textdata)
{
	struct sfq_value val;

	bzero(&val, sizeof(val));

/*
(char*) cast for show-off warning
 */
	val.execpath = (char*)execpath;
	val.execargs = (char*)execargs;
	val.metadata = (char*)metadata;
	val.payload_type = SFQ_PLT_CHARARRAY | SFQ_PLT_NULLTERM;
	val.payload_size = (textdata ? (strlen(textdata) + 1) : 0);
	val.payload = (sfq_byte*)textdata;

	return sfq_push(querootdir, quename, &val);
}

int sfq_push_bin(const char* querootdir, const char* quename, const char* execpath, const char* execargs, const char* metadata, const sfq_byte* payload, size_t payload_size)
{
	struct sfq_value val;

	bzero(&val, sizeof(val));

/*
(char*) cast for show-off warning
 */
	val.execpath = (char*)execpath;
	val.execargs = (char*)execargs;
	val.metadata = (char*)metadata;
	val.payload_type = SFQ_PLT_BINARY;
	val.payload_size = payload_size;
	val.payload = (sfq_byte*)payload;

	return sfq_push(querootdir, quename, &val);
}


bool sfq_copy_val2ioeb(const struct sfq_value* val, struct sfq_ioelm_buff* ioeb)
{
LIBFUNC_INITIALIZE

	size_t eh_size = 0;
	size_t add_all = 0;
	sfq_uchar elmmargin_ = 0;

	if (! ioeb)
	{
		FIRE(SFQ_RC_EA_FUNCARG, "ioeb");
	}

	if (! val)
	{
		FIRE(SFQ_RC_EA_FUNCARG, "val");
	}

/* initialize */
	eh_size = sizeof(ioeb->eh);

	bzero(ioeb, sizeof(*ioeb));

/* */
	ioeb->eh.eh_size = eh_size;
	ioeb->eh.id = val->id;
	ioeb->eh.pushtime = val->pushtime;
	ioeb->eh.payload_type = val->payload_type;

	if (val->execpath)
	{
		size_t execpath_len = strlen(val->execpath);
		if (execpath_len)
		{
			size_t execpath_size = execpath_len + 1;

			if (execpath_size >= USHRT_MAX)
			{
				FIRE(SFQ_RC_EA_OVERLIMIT, "execpath_size");
			}
			if (execpath_size >= PATH_MAX)
			{
				FIRE(SFQ_RC_EA_OVERLIMIT, "execpath_size");
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
				FIRE(SFQ_RC_EA_OVERLIMIT, "execargs_size");
			}

			if (sysmax > 0)
			{
				if (execargs_size >= (size_t)sysmax)
				{
					FIRE(SFQ_RC_EA_OVERLIMIT, "execargs_size");
				}
			}

			ioeb->execargs = val->execargs;
			ioeb->eh.execargs_size = (uint)execargs_size;
		}
	}

	if (val->metadata)
	{
		size_t metadata_len = strlen(val->metadata);
		if (metadata_len)
		{
			size_t metadata_size = metadata_len + 1;

			if (metadata_size >= USHRT_MAX)
			{
				FIRE(SFQ_RC_EA_OVERLIMIT, "metadata_size");
			}

			ioeb->metadata = val->metadata;
			ioeb->eh.metadata_size = (ushort)metadata_size;
		}
	}

	if (val->payload && val->payload_size)
	{
		ioeb->eh.payload_size = val->payload_size;
		ioeb->payload = val->payload;
	}

/* for debug */
	add_all =
	(
		eh_size +
		ioeb->eh.execpath_size +
		ioeb->eh.execargs_size +
		ioeb->eh.metadata_size +
		ioeb->eh.payload_size
	);

	elmmargin_ = SFQ_ALIGN_MARGIN(add_all);

	ioeb->eh.elmsize_ = add_all + elmmargin_;
	ioeb->eh.elmmargin_ = elmmargin_;

LIBFUNC_COMMIT

LIBFUNC_FINALIZE

	return LIBFUNC_IS_SUCCESS();
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

	val->execpath = ioeb->eh.execpath_size ? ioeb->execpath : NULL;
	val->execargs = ioeb->eh.execargs_size ? ioeb->execargs : NULL;
	val->metadata = ioeb->eh.metadata_size ? ioeb->metadata : NULL;

	val->payload_type = ioeb->eh.payload_type;
	val->payload_size = ioeb->eh.payload_size;
	val->payload = ioeb->payload;

	return true;
}

void sfq_free_value(struct sfq_value* val)
{
	if (! val)
	{
		return;
	}

	free(val->execpath);
	free(val->execargs);
	free(val->metadata);
	free(val->payload);

	bzero(val, sizeof(*val));
}

int sfq_alloc_print_value(const struct sfq_value* val, struct sfq_value* dst)
{
LIBFUNC_INITIALIZE

	const char* na = "N/A";

	char* execpath = NULL;
	char* execargs = NULL;
	char* metadata = NULL;
	char* payload = NULL;

/* */
	if (! val)
	{
		FIRE(SFQ_RC_EA_FUNCARG, "val");
	}

	if (! dst)
	{
		FIRE(SFQ_RC_EA_FUNCARG, "dst");
	}

	bzero(dst, sizeof(*dst));

	execpath = strdup(val->execpath ? val->execpath : na);
	if (! execpath)
	{
		FIRE(SFQ_RS_ES_STRDUP, "execpath");
	}

	execargs = strdup(val->execargs ? val->execargs : na);
	if (! execargs)
	{
		FIRE(SFQ_RS_ES_STRDUP, "execargs");
	}

	metadata = strdup(val->metadata ? val->metadata : na);
	if (! metadata)
	{
		FIRE(SFQ_RS_ES_STRDUP, "metadata");
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
			payload = strdup("* BINARY *");
		}
	}
	else
	{
		payload = strdup(na);
	}

	if (! payload)
	{
		FIRE(SFQ_RS_ES_STRDUP, "payload");
	}

	dst->id = val->id;
	dst->pushtime = val->pushtime;
	dst->payload_type = val->payload_type;
	dst->payload_size = val->payload_size;

	dst->execpath = execpath;
	dst->execargs = execargs;
	dst->metadata = metadata;
	dst->payload = (sfq_byte*)payload;

LIBFUNC_COMMIT

	if (LIBFUNC_IS_ROLLBACK())
	{
		free(execpath);
		free(execargs);
		free(metadata);
		free(payload);
	}

LIBFUNC_FINALIZE

	return LIBFUNC_RC();
}

struct sfq_open_names* sfq_alloc_open_names(const char* querootdir, const char* quename)
{
LIBFUNC_INITIALIZE

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
	if (! querootdir)
	{
		querootdir = SFQ_DEFAULT_QUEUE_DIR;
	}

	if (! quename)
	{
		quename = SFQ_DEFAULT_QUEUE_NAME;
	}

	quename_len = strlen(quename);
	if (quename_len == 0)
	{
		FIRE(SFQ_RC_EA_FUNCARG, "quename_len == 0");
	}

/* save rootdir realpath */
	om_querootdir = realpath(querootdir, NULL);
	if (! om_querootdir)
	{
		FIRE(SFQ_RS_ES_REALPATH, "om_realpath");
	}

/*
querootdir は相対パスの可能性があるので、以降では参照せず
om_querootdir を使うこと
*/

	om_querootdir_len = strlen(om_querootdir);
	if (om_querootdir_len == 0)
	{
		FIRE(SFQ_RC_EA_FUNCARG, "om_querootdir_len == 0");
	}

/* quedir */
	/* "ROOT/NAME\0" */
	quedir_len = om_querootdir_len + 1 + quename_len;
	quedir_size = quedir_len + 1;
	if (quedir_size >= PATH_MAX)
	{
		FIRE(SFQ_RC_EA_OVERLIMIT, "quedir_size");
	}

	quedir = malloc(quedir_size);
	if (! quedir)
	{
		FIRE(SFQ_RC_ES_MEMALLOC, "quedir");
	}
	snprintf(quedir, quedir_size, "%s/%s", om_querootdir, quename);

/* quefile */
	/* "QDIR/filename\0" */
	quefile_len = quedir_len + 1 + strlen(SFQ_QUEUE_FILENAME);
	quefile_size = quefile_len + 1;
	if (quefile_size >= PATH_MAX)
	{
		FIRE(SFQ_RC_EA_OVERLIMIT, "quefile_size");
	}

	quefile = malloc(quefile_size);
	if (! quefile)
	{
		FIRE(SFQ_RC_ES_MEMALLOC, "quefile");
	}
	snprintf(quefile, quefile_size, "%s/%s", quedir, SFQ_QUEUE_FILENAME);

/* quelogdir */
	/* "QDIR/logdir\0" */
	quelogdir_len = quedir_len + 1 + strlen(SFQ_QUEUE_LOGDIRNAME);
	quelogdir_size = quelogdir_len + 1;
	if (quelogdir_size >= PATH_MAX)
	{
		FIRE(SFQ_RC_EA_OVERLIMIT, "quelogdir_size");
	}

	quelogdir = malloc(quelogdir_size);
	if (! quelogdir)
	{
		FIRE(SFQ_RC_ES_MEMALLOC, "quelogdir");
	}
	snprintf(quelogdir, quelogdir_size, "%s/%s", quedir, SFQ_QUEUE_LOGDIRNAME);

/* queproclogdir */
	/* "QLDIR/proc\0 */
	queproclogdir_size = quelogdir_len + 1 + strlen(SFQ_QUEUE_PROC_LOGDIRNAME) + 1;
	if (queproclogdir_size >= PATH_MAX)
	{
		FIRE(SFQ_RC_EA_OVERLIMIT, "queproclogdir_size");
	}

	queproclogdir = malloc(queproclogdir_size);
	if (! queproclogdir)
	{
		FIRE(SFQ_RC_ES_MEMALLOC, "queproclogdir");
	}
	snprintf(queproclogdir, queproclogdir_size, "%s/%s", quelogdir, SFQ_QUEUE_PROC_LOGDIRNAME);

/* queexeclogdir */
	/* "QLDIR/exec\0 */
	queexeclogdir_size = quelogdir_len + 1 + strlen(SFQ_QUEUE_EXEC_LOGDIRNAME) + 1;
	if (queexeclogdir_size >= PATH_MAX)
	{
		FIRE(SFQ_RC_EA_OVERLIMIT, "queexeclogdir_size");
	}

	queexeclogdir = malloc(queexeclogdir_size);
	if (! queexeclogdir)
	{
		FIRE(SFQ_RC_ES_MEMALLOC, "queexeclogdir");
	}
	snprintf(queexeclogdir, queexeclogdir_size, "%s/%s", quelogdir, SFQ_QUEUE_EXEC_LOGDIRNAME);

/* semname */
	/* "/NAME\0" */
	semname = strdup(quefile);
	if (! semname)
	{
		FIRE(SFQ_RC_ES_MEMALLOC, "semname");
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
		FIRE(SFQ_RS_ES_STRDUP, "om_quename");
	}

/* */
	om = malloc(sizeof(*om));
	if (! om)
	{
		FIRE(SFQ_RC_ES_MEMALLOC, "om");
	}

	om->querootdir = om_querootdir;
	om->quename = om_quename;
	om->quedir = quedir;
	om->quefile = quefile;
	om->quelogdir = quelogdir;
	om->queproclogdir = queproclogdir;
	om->queexeclogdir = queexeclogdir;
	om->semname = semname;

LIBFUNC_COMMIT

	if (LIBFUNC_IS_ROLLBACK())
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

LIBFUNC_FINALIZE

	return om;
}

void sfq_free_open_names(struct sfq_open_names* om)
{
	if (om)
	{
		free(om->querootdir);
		free(om->quename);
		free(om->quedir);
		free(om->quefile);
		free(om->quelogdir);
		free(om->queproclogdir);
		free(om->queexeclogdir);
		free(om->semname);
		free(om);
	}
}
