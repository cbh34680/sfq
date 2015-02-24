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

static sfq_bool g_printOnOff = SFQ_true;

void sfq_set_print(sfq_bool printOnOff)
{
	g_printOnOff = printOnOff;
}

sfq_bool sfq_get_print()
{
	return g_printOnOff;
}

pid_t sfq_gettid(void)
{
	return syscall(SYS_gettid);
}

/*
https://github.com/dotcloud/lxc/blob/master/src/lxc/caps.c
*/
sfq_bool sfq_caps_isset(cap_value_t cap)
{
	sfq_bool ret = SFQ_false;

#ifdef __GNUC__
	cap_t cap_p = NULL;

	cap_p = cap_get_pid(getpid());
	if (cap_p)
	{
		int irc = -1;
		cap_flag_value_t flag = 0;

		irc = cap_get_flag(cap_p, cap, CAP_EFFECTIVE, &flag);
		if (irc == 0)
		{
			ret = (flag == CAP_SET);
		}

		cap_free(cap_p);
	}
#endif

	return ret;
}

sfq_bool sfq_pwdgrp_id2nam_alloc(uid_t usrid, gid_t grpid,
	const char** usrnam_ptr, const char** grpnam_ptr)
{
	long sysmax = 0;
	char* buf = NULL;
	size_t bufsize = 0;

	char* usrnam = NULL;
	char* grpnam = NULL;

SFQ_LIB_ENTER

	sysmax = sysconf(SFQ_MAX(_SC_GETPW_R_SIZE_MAX, _SC_GETGR_R_SIZE_MAX));
	if (sysmax > 0)
	{
		bufsize = (size_t)sysmax;
	}
	else
	{
		bufsize = 1024U;
	}

	buf = alloca(bufsize);
	if (! buf)
	{
		SFQ_FAIL(ES_MEMORY, "ALLOC(buf)");
	}

/* usrid */
	if (SFQ_ISSET_UID(usrid))
	{
		struct passwd pwd;
		struct passwd* result = NULL;

		getpwuid_r(usrid, &pwd, buf, bufsize, &result);
		if (! result)
		{
			SFQ_FAIL(ES_MEMORY, "specified uid not found");
		}

		if (usrnam_ptr)
		{
			usrnam = strdup(pwd.pw_name);
			if (! usrnam)
			{
				SFQ_FAIL(ES_MEMORY, "strdup(pw_name)");
			}

			(*usrnam_ptr) = usrnam;
		}
	}

/* grpid */
	if (SFQ_ISSET_GID(grpid))
	{
		struct group grp;
		struct group* result = NULL;

		getgrgid_r(grpid, &grp, buf, bufsize, &result);
		if (! result)
		{
			SFQ_FAIL(ES_MEMORY, "specified gid not found");
		}

		if (grpnam_ptr)
		{
			grpnam = strdup(grp.gr_name);
			if (! grpnam)
			{
				SFQ_FAIL(ES_MEMORY, "strdup(gr_name)");
			}

			(*grpnam_ptr) = grpnam;
		}
	}


SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		free(usrnam);
		usrnam = NULL;

		free(grpnam);
		grpnam = NULL;
	}


SFQ_LIB_LEAVE

	return SFQ_LIB_IS_SUCCESS();
}

sfq_bool sfq_pwdgrp_nam2id(const char* usrnam, const char* grpnam,
	uid_t* usrid_ptr, gid_t* grpid_ptr)
{
	long sysmax = 0;
	char* buf = NULL;
	size_t bufsize = 0;

SFQ_LIB_ENTER

	sysmax = sysconf(SFQ_MAX(_SC_GETPW_R_SIZE_MAX, _SC_GETGR_R_SIZE_MAX));
	if (sysmax > 0)
	{
		bufsize = (size_t)sysmax;
	}
	else
	{
		bufsize = 1024U;
	}

	buf = alloca(bufsize);
	if (! buf)
	{
		SFQ_FAIL(ES_MEMORY, "ALLOC(buf)");
	}

/* user */
	if (usrnam)
	{
		struct passwd pwd;
		struct passwd* result = NULL;

		getpwnam_r(usrnam, &pwd, buf, bufsize, &result);
		if (! result)
		{
			SFQ_FAIL(ES_MEMORY, "specified user not found");
		}

		if (usrid_ptr)
		{
			(*usrid_ptr) = pwd.pw_uid;
		}
	}

/* group */
	if (grpnam)
	{
		struct group grp;
		struct group* result = NULL;

		getgrnam_r(grpnam, &grp, buf, bufsize, &result);
		if (! result)
		{
			SFQ_FAIL(ES_MEMORY, "specified group not found");
		}

		if (grpid_ptr)
		{
			(*grpid_ptr) = grp.gr_gid;
		}
	}

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	return SFQ_LIB_IS_SUCCESS();
}

size_t sfq_payload_len(const struct sfq_value* val)
{
	size_t ret = 0;

	if (val)
	{
		if (val->payload)
		{
			ret = val->payload_size;

			if ((val->payload_type & SFQ_PLT_NULLTERM) &&
			    (val->payload_type & SFQ_PLT_CHARARRAY))
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

void sfq_init_ioeb(struct sfq_ioelm_buff* ioeb)
{
	assert(ioeb);

	bzero(ioeb, sizeof(*ioeb));
}

#define VAL2IOEB_SET_NTSTR(key_, maxsiz_) \
	\
	if (val->key_) \
	{ \
		size_t len_ = strlen(val->key_); \
		if (len_) \
		{ \
			size_t siz_ = len_ + 1; \
			if (siz_ >= (size_t)maxsiz_) \
			{ \
				SFQ_FAIL(EA_OVERLIMIT, "siz_ >= maxsiz_"); \
			} \
			ioeb->key_ = val->key_; \
			ioeb->eh.key_ ## _size = siz_; \
		} \
	}


sfq_bool sfq_copy_val2ioeb(const struct sfq_value* val, struct sfq_ioelm_buff* ioeb)
{
SFQ_LIB_ENTER

	size_t add_all = 0;
	sfq_uchar elmmargin_ = 0;

	long sc_arg_max = 0;

	sc_arg_max = sysconf(_SC_ARG_MAX);
	if (sc_arg_max <= 0)
	{
		sc_arg_max = UINT_MAX;
	}

	if (! ioeb)
	{
		SFQ_FAIL(EA_FUNCARG, "ioeb");
	}

	if (! val)
	{
		SFQ_FAIL(EA_FUNCARG, "val");
	}

/* initialize */
	sfq_init_ioeb(ioeb);

/* */
	ioeb->eh.eh_size = sizeof(ioeb->eh);

	ioeb->eh.id = val->id;
	ioeb->eh.pushtime = val->pushtime;
	ioeb->eh.disabled = val->disabled;

	uuid_copy(ioeb->eh.uuid, val->uuid);

/*
payload, payload_size, payload_type は必ず同期する
*/
	if (val->payload)
	{
		size_t payload_size = 0;

		if (! val->payload_type)
		{
			SFQ_FAIL(EA_NOTPAYLOAD, "payload_type is not set");
		}

		payload_size = val->payload_size;

		if (! payload_size)
		{
			if (val->payload_type & (SFQ_PLT_CHARARRAY | SFQ_PLT_NULLTERM))
			{
/*
null-term 文字列の場合に payload_size が未設定の場合は自動算出
*/
				payload_size = strlen((char*)val->payload) + 1;
			}
			else
			{
				SFQ_FAIL(EA_NOTPAYLOAD, "payload_size is not set");
			}
		}

		ioeb->payload = val->payload;
		ioeb->eh.payload_type = val->payload_type;
		ioeb->eh.payload_size = payload_size;
	}
	else
	{
		if (val->payload_size || val->payload_type)
		{
			SFQ_FAIL(EA_NOTPAYLOAD, "payload is not set [size=%zu] [type=%u]",
				val->payload_size, val->payload_type);
		}
	}

/* */
	VAL2IOEB_SET_NTSTR(eworkdir,  SFQ_MIN(USHRT_MAX, PATH_MAX));
	VAL2IOEB_SET_NTSTR(execpath,  SFQ_MIN(USHRT_MAX, PATH_MAX));
	VAL2IOEB_SET_NTSTR(execargs,  SFQ_MIN(UINT_MAX, sc_arg_max));
	VAL2IOEB_SET_NTSTR(metatext,  USHRT_MAX);
	VAL2IOEB_SET_NTSTR(soutpath,  SFQ_MIN(USHRT_MAX, PATH_MAX));
	VAL2IOEB_SET_NTSTR(serrpath,  SFQ_MIN(USHRT_MAX, PATH_MAX));

/* for debug */
	add_all =
	(
		sizeof(ioeb->eh) +
		ioeb->eh.payload_size  +
		ioeb->eh.eworkdir_size +
		ioeb->eh.execpath_size +
		ioeb->eh.execargs_size +
		ioeb->eh.metatext_size +
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

#define IOEB2VAL_SET_NTSTR(key_) \
\
	if (ioeb->eh.key_ ## _size) \
	{ \
		assert(ioeb->key_); \
		val->key_ = ioeb->key_; \
	}

sfq_bool sfq_copy_ioeb2val(const struct sfq_ioelm_buff* ioeb, struct sfq_value* val)
{
	if (! ioeb)
	{
		return SFQ_false;
	}

	if (! val)
	{
		return SFQ_false;
	}

	bzero(val, sizeof(*val));

	val->id = ioeb->eh.id;
	val->pushtime = ioeb->eh.pushtime;
	val->disabled = ioeb->eh.disabled;
	val->elmsize_ = ioeb->eh.elmsize_;

	uuid_copy(val->uuid, ioeb->eh.uuid);

/* */
	if (ioeb->eh.payload_size)
	{
		assert(ioeb->eh.payload_type);
		assert(ioeb->payload);

		val->payload_size = ioeb->eh.payload_size;
		val->payload_type = ioeb->eh.payload_type;
		val->payload = ioeb->payload;
	}

/* */
	IOEB2VAL_SET_NTSTR(eworkdir);
	IOEB2VAL_SET_NTSTR(execpath);
	IOEB2VAL_SET_NTSTR(execargs);
	IOEB2VAL_SET_NTSTR(metatext);
	IOEB2VAL_SET_NTSTR(soutpath);
	IOEB2VAL_SET_NTSTR(serrpath);

	return SFQ_true;
}

void sfq_free_value(struct sfq_value* val)
{
	if (! val)
	{
		return;
	}

	free((char*)val->querootdir);
	free((char*)val->quename);
	free((char*)val->payload);
	free((char*)val->eworkdir);
	free((char*)val->execpath);
	free((char*)val->execargs);
	free((char*)val->metatext);
	free((char*)val->soutpath);
	free((char*)val->serrpath);

	bzero(val, sizeof(*val));
}

int sfq_alloc_print_value(const struct sfq_value* val, struct sfq_value* dst)
{
SFQ_LIB_ENTER

	const char* NA = "N/A";

	char* querootdir = NULL;
	char* quename = NULL;
	char* eworkdir = NULL;
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

/* */
	if (val->payload)
	{
		assert(val->payload_type);
		assert(val->payload_size);

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
		SFQ_FAIL(ES_MEMORY, "payload");
	}

/* */
	querootdir = strdup(val->querootdir ? val->querootdir : NA);
	if (! querootdir)
	{
		SFQ_FAIL(ES_MEMORY, "querootdir");
	}

	quename = strdup(val->quename ? val->quename : NA);
	if (! quename)
	{
		SFQ_FAIL(ES_MEMORY, "quename");
	}

	eworkdir = strdup(val->eworkdir ? val->eworkdir : NA);
	if (! eworkdir)
	{
		SFQ_FAIL(ES_MEMORY, "eworkdir");
	}

	execpath = strdup(val->execpath ? val->execpath : NA);
	if (! execpath)
	{
		SFQ_FAIL(ES_MEMORY, "execpath");
	}

	execargs = strdup(val->execargs ? val->execargs : NA);
	if (! execargs)
	{
		SFQ_FAIL(ES_MEMORY, "execargs");
	}

	metatext = strdup(val->metatext ? val->metatext : NA);
	if (! metatext)
	{
		SFQ_FAIL(ES_MEMORY, "metatext");
	}

	soutpath = strdup(val->soutpath ? val->soutpath : NA);
	if (! soutpath)
	{
		SFQ_FAIL(ES_MEMORY, "soutpath");
	}

	serrpath = strdup(val->serrpath ? val->serrpath : NA);
	if (! serrpath)
	{
		SFQ_FAIL(ES_MEMORY, "serrpath");
	}

/* */
	dst->id = val->id;
	dst->pushtime = val->pushtime;
	dst->disabled = val->disabled;
	dst->elmsize_ = val->elmsize_;

	uuid_copy(dst->uuid, val->uuid);

	dst->payload_type = val->payload_type;
	dst->payload_size = val->payload_size;
	dst->payload = (sfq_byte*)payload;

	dst->querootdir = querootdir;
	dst->quename = quename;
	dst->eworkdir = eworkdir;
	dst->execpath = execpath;
	dst->execargs = execargs;
	dst->metatext = metatext;
	dst->soutpath = soutpath;
	dst->serrpath = serrpath;

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		free(payload);
		free(querootdir);
		free(quename);
		free(eworkdir);
		free(execpath);
		free(execargs);
		free(metatext);
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
		querootdir = getenv("SFQ_QUEROOTDIR");
		if (! querootdir)
		{
			querootdir = SFQ_DEFAULT_QUEROOTDIR;
		}
	}

	if (! quename)
	{
		quename = getenv("SFQ_QUENAME");
		if (! quename)
		{
			quename = SFQ_DEFAULT_QUENAME;
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
		SFQ_FAIL(ES_PATH, "%s: resolv realpath", querootdir);
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
		SFQ_FAIL(ES_MEMORY, "quedir");
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
		SFQ_FAIL(ES_MEMORY, "quefile");
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
		SFQ_FAIL(ES_MEMORY, "quelogdir");
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
		SFQ_FAIL(ES_MEMORY, "queproclogdir");
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
		SFQ_FAIL(ES_MEMORY, "queexeclogdir");
	}
	snprintf(queexeclogdir, queexeclogdir_size, "%s/%s", quelogdir, SFQ_QUEUE_EXEC_LOGDIRNAME);

/* semname */
	/* "/NAME\0" */
	semname = strdup(quefile);
	if (! semname)
	{
		SFQ_FAIL(ES_MEMORY, "semname");
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
		SFQ_FAIL(ES_MEMORY, "om_quename");
	}

/* */
	om = malloc(sizeof(*om));
	if (! om)
	{
		SFQ_FAIL(ES_MEMORY, "om");
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
sfq_bool sfq_mkdir_p(const char *arg, mode_t perm)
{
	char* dir = NULL;
	char *p = NULL;
	size_t len = 0;


	dir = sfq_stradup(arg);
	if (! dir)
	{
		return SFQ_false;
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
					return SFQ_false;
				}
			}
			*p = '/';
		}
	}

	if (mkdir(dir, perm) == -1)
	{
		if (errno != EEXIST)
		{
			return SFQ_false;
		}
	}

	return SFQ_true;
}

char* sfq_alloc_concat_NT(const char* first, ...)
{
	size_t len = 0;
	char* ret = NULL;

	va_list ap;
	va_list copy;

	const char* curr = NULL;

	va_start(ap, first);
	va_copy(copy, ap);

	curr = first;
	while (curr)
	{
		len += strlen(curr);
		curr = va_arg(ap, const char*);
	}

	va_end(ap);

	ret = malloc(len + 1);
	if (! ret)
	{
		return NULL;
	}
	ret[0] = '\0';

	curr = first;
	while (curr)
	{
		strcat(ret, curr);
		curr = va_arg(copy, const char*);
	}

	va_end(copy);

	return ret;
}

static sfq_bool is_in(char c, const char* pos)
{
	sfq_bool ret = SFQ_false;

	if (pos)
	{
		while (*pos)
		{
			if (c == (*pos))
			{
				ret = SFQ_true;
				break;
			}

			pos++;
		}
	}

	return ret;
}

size_t sfq_rtrim(char* str, const char* cmask)
{
	size_t nchange = 0;
	char* pos = NULL;

	if (str)
	{
		pos = &str[strlen(str)];

		do
		{
			pos--;

			if (cmask && cmask[0])
			{
				if (is_in(*pos, cmask))
				{
					(*pos) = '\0';
					nchange++;

					continue;
				}
			}
			else
			{
				if (isspace(*pos))
				{
					(*pos) = '\0';
					nchange++;

					continue;
				}
			}

			break;
		}
		while (pos != str);
	}

	return nchange;
}

int sfq_count_char(char delim, const char* searchstr)
{
	int cnt = -1;

	if (searchstr)
	{
		const char* pos = NULL;

		cnt = 0;
		pos = searchstr;
		while (*pos)
		{
			if ((*pos) == delim)
			{
				cnt++;
			}
			pos++;
		}
	}

	return cnt;
}

char** sfq_alloc_split(char delim, const char* orgstr, int* num_ptr)
{
	char** ret = NULL;
	int loop_max = 0;
	int num = 0;
	char delimiter_str[] = { delim, '\0' };

	char* copy = NULL;

	char* pos = NULL;
	char* saveptr = NULL;
	char* token = NULL;
	int i = 0;

SFQ_LIB_ENTER

	if (! orgstr)
	{
		SFQ_FAIL(EA_FUNCARG, "orgstr is null");
	}

	copy = sfq_stradup(orgstr);
	if (! copy)
	{
		SFQ_FAIL(ES_MEMORY, "sfq_stradup(orgstr)");
	}

	loop_max = sfq_count_char(delim, orgstr);
	if (loop_max < 0)
	{
		SFQ_FAIL(EA_ASSERT, "sfq_count_char");
	}
	loop_max++;

	ret = calloc(sizeof(char*), loop_max + 1);
	if (! ret)
	{
		SFQ_FAIL(ES_MEMORY, "calloc");
	}

	for (i=0, pos=copy; i<loop_max; i++, pos=NULL)
	{
		char* dupstr = NULL;

		token = strtok_r(pos, delimiter_str, &saveptr);
		if (! token)
		{
			break;
		}

		dupstr = strdup(token);
		if (! dupstr)
		{
			SFQ_FAIL(ES_MEMORY, "strdup(token)");
		}

		ret[num++] = dupstr;
	}

	if (num_ptr)
	{
		(*num_ptr) = num;
	}

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		sfq_free_strarr(ret);
		ret = NULL;
	}

SFQ_LIB_LEAVE

	return ret;
}

void sfq_free_strarr(char** strarr)
{
	if (strarr)
	{
		char** pos = strarr;

		while (*pos)
		{
			free(*pos);
			(*pos) = NULL;

			pos++;
		}

		free(strarr);
		strarr = NULL;
	}
}

