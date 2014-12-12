#include "sfq-lib.h"

static bool pwd_nam2id(const char* queuser, const char* quegroup,
	uid_t* queuserid_ptr, gid_t* quegroupid_ptr);

/*
https://github.com/dotcloud/lxc/blob/master/src/lxc/caps.c
*/
static bool caps_isset(cap_value_t cap)
{
	bool ret = false;

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

int sfq_init(const char* querootdir, const char* quename, const struct sfq_queue_init_params* qip)
{
SFQ_LIB_INITIALIZE

	struct sfq_queue_object* qo = NULL;
	struct sfq_process_info* procs = NULL;

	ushort qfh_size = 0;
	size_t eh_size = 0;
	size_t pi_size = 0;
	size_t procs_size = 0;
	size_t elmseg_start_pos = 0;
	bool b = false;

	uid_t euid = (uid_t)-1;
	gid_t egid = (uid_t)-1;
	uid_t queuserid = (uid_t)-1;
	gid_t quegroupid = (uid_t)-1;
	bool chmod_GaW = false;

	struct sfq_queue_create_params qcp;
	struct sfq_file_header qfh;

/* initialize */
	errno = 0;

	qfh_size = sizeof(qfh);
	eh_size = sizeof(struct sfq_e_header);
	pi_size = sizeof(struct sfq_process_info);

	euid = geteuid();
	egid = getegid();

	bzero(&qcp, sizeof(qcp));
	bzero(&qfh, qfh_size);

/*
queue header の初期値を設定
*/

	strcpy(qfh.magicstr, SFQ_MAGICSTR);
	qfh.qfh_size = qfh_size;
	qfh.qh.dval.questate = qip->questate;
	strcpy(qfh.last_qhd1.lastoper, "---");
	strcpy(qfh.last_qhd2.lastoper, "---");

/*
"-U", "-G" が有効なのは euid == 0(root) のときのみ
*/
	SFQ_UNSET_UID(queuserid);
	SFQ_UNSET_GID(quegroupid);

	b = pwd_nam2id(qip->queuser, qip->quegroup, &queuserid, &quegroupid);
	if (! b)
	{
		SFQ_FAIL(EA_PWDNAME2ID, "pwd_nam2id");
	}

	if (caps_isset(CAP_CHOWN))
	{
		if (SFQ_ISSET_UID(queuserid))
		{
			if (SFQ_ISSET_GID(quegroupid))
			{
				/* chown queuserid.quegroupid */
				/* chmod g+w */

				chmod_GaW= true;
			}
			else
			{
				/* chown queuserid.root */
			}
		}
		else if (SFQ_ISSET_GID(quegroupid))
		{
			/* chown root.quegroupid */
			/* chmod g+w */

			chmod_GaW= true;
		}
		else
		{
/*
root の場合、"-U" か "-G" の指定があるときのみ通過させる
--> root.root のキューは作成できない
*/
			SFQ_FAIL(EA_NOTPERMIT, "specify either the user or group");
		}

	}
	else
	{
/*
一般ユーザが "-U" "-G" を指定しても、euid, egid と同じであれば通過させる
*/
		if (SFQ_ISSET_UID(queuserid))
		{
			if (queuserid != euid)
			{
				SFQ_FAIL(EA_NOTPERMIT, "specified user, operation not permitted");
			}
		}

		if (SFQ_ISSET_GID(quegroupid))
		{
			if (quegroupid != egid)
			{
				SFQ_FAIL(EA_NOTPERMIT, "specified group, operation not permitted");
			}

			/* chmod g+w */

			chmod_GaW = true;
		}

		SFQ_UNSET_UID(queuserid);
		SFQ_UNSET_GID(quegroupid);
	}

/* check process num */
	if (qip->procs_num < qip->boota_proc_num)
	{
		SFQ_FAIL(EA_OVERLIMIT, "bootable process must not be larger than process num");
	}

	if (qip->procs_num)
	{
		long sysmax = sysconf(_SC_CHILD_MAX);
		if (sysmax > 0)
		{
			if (qip->procs_num > sysmax)
			{
				SFQ_FAIL(EA_OVERLIMIT, "process num is greater than system limit");
			}
		}

/* calc procs-start offset */
		procs_size = pi_size * qip->procs_num;

		procs = alloca(procs_size);
		if (! procs)
		{
			SFQ_FAIL(ES_MEMALLOC, "ALLOC(procs)");
		}

		bzero(procs, procs_size);

		qfh.qh.sval.procseg_start_pos = qfh_size;
	}

/* set element-start offset */
	elmseg_start_pos = qfh_size + procs_size;

/* check size */
	if (qip->filesize_limit < (elmseg_start_pos + eh_size + 1))
	{
	/* 最小ファイルサイズは sfq_file_header + pid_table + sfq_e_header + payload(1 byte) */

		SFQ_FAIL(EA_FSIZESMALL, "specified filesize-limit too small");
	}

/* open queue-file */
	qcp.querootdir = querootdir;
	qcp.quename = quename;
	qcp.queuserid = queuserid;
	qcp.quegroupid = quegroupid;
	qcp.chmod_GaW = chmod_GaW;

	qo = sfq_create_queue(&qcp);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENFILE, "sfq_create_queue");
	}

	if (procs)
	{
		if (qip->procs_num > qip->boota_proc_num)
		{
			int i = 0;

/*
起動可能数を超えたスロットはロックしておく

--> 配列の後方からロックする
*/
			for (i=0; i<(qip->procs_num - qip->boota_proc_num); i++)
			{
				struct sfq_process_info* proc = &procs[(qip->procs_num - 1) - i];

				proc->procstate = SFQ_PIS_LOCK;
				proc->updtime = qo->opentime;
			}
		}
	}

/* initialize queue-header */

	/* 静的属性値の設定 */
	qfh.qh.sval.elmseg_start_pos = elmseg_start_pos;
	qfh.qh.sval.elmseg_end_pos = (qip->filesize_limit - 1);
	qfh.qh.sval.filesize_limit = qip->filesize_limit;
	qfh.qh.sval.payloadsize_limit = qip->payloadsize_limit;
	qfh.qh.sval.procs_num = qip->procs_num;

/*
各ポジションの初期化

静的属性値の値から算出しているので sfq_qh_init_pos() の位置は動かさないこと
*/
	sfq_qh_init_pos(&qfh.qh);

/* write queue-header */
	b = sfq_writeqfh(qo, &qfh, procs, "INI");
	if (! b)
	{
		SFQ_FAIL(EA_WRITEQFH, "sfq_writeqfh");
	}

/* print queue header */
	sfq_print_qo(qo);
	sfq_print_qf_header(&qfh);

SFQ_LIB_CHECKPOINT

SFQ_LIB_FINALIZE

	sfq_close_queue(qo);
	qo = NULL;

	return SFQ_LIB_RC();
}

static bool pwd_nam2id(const char* queuser, const char* quegroup,
	uid_t* queuserid_ptr, gid_t* quegroupid_ptr)
{
	long sysmax = 0;
	char* buf = NULL;
	size_t bufsize = 0;

SFQ_LIB_INITIALIZE

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
		SFQ_FAIL(ES_MEMALLOC, "ALLOC(buf)");
	}

/* user */
	if (queuser)
	{
		struct passwd pwd;
		struct passwd *result;

		getpwnam_r(queuser, &pwd, buf, bufsize, &result);
		if (! result)
		{
			SFQ_FAIL(ES_MEMALLOC, "specified user not found");
		}

#ifdef SFQ_DEBUG_BUILD
		fprintf(stderr, "user[%s] = %u\n", queuser, pwd.pw_uid);
#endif

		(*queuserid_ptr) = pwd.pw_uid;
	}

/* group */
	if (quegroup)
	{
		struct group grp;
		struct group *result;

		getgrnam_r(quegroup, &grp, buf, bufsize, &result);
		if (! result)
		{
			SFQ_FAIL(ES_MEMALLOC, "specified group not found");
		}

#ifdef SFQ_DEBUG_BUILD
		fprintf(stderr, "group[%s] = %u\n", quegroup, grp.gr_gid);
#endif

		(*quegroupid_ptr) = grp.gr_gid;
	}

SFQ_LIB_CHECKPOINT

SFQ_LIB_FINALIZE

	return SFQ_LIB_IS_SUCCESS();
}

