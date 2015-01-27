#include "sfq-lib.h"

int sfq_init(const char* querootdir, const char* quename, const struct sfq_queue_init_params* qip)
{
SFQ_ENTP_ENTER

	struct sfq_queue_object* qo = NULL;
	struct sfq_process_info* procs = NULL;

	ushort qfh_size = 0;
	size_t eh_size = 0;
	size_t pi_size = 0;
	size_t procs_size = 0;
	size_t elmseg_start_pos = 0;
	sfq_bool b = SFQ_false;
	sfq_bool chmod_GaW = SFQ_false;

	uid_t euid = SFQ_UID_NONE;
	gid_t egid = SFQ_GID_NONE;
	uid_t queusrid = SFQ_UID_NONE;
	gid_t quegrpid = SFQ_GID_NONE;

	struct sfq_queue_create_params qcp;
	struct sfq_file_header qfh;

/* initialize */
	qfh_size = sizeof(qfh);
	eh_size = sizeof(struct sfq_e_header);
	pi_size = sizeof(struct sfq_process_info);

	bzero(&qcp, sizeof(qcp));
	bzero(&qfh, qfh_size);

	euid = geteuid();
	egid = getegid();

/*
queue header の初期値を設定
*/

	strcpy(qfh.qfs.magicstr, SFQ_MAGICSTR);
	qfh.qfs.qfh_size = qfh_size;

	qfh.qh.dval.questate = qip->questate;
	strcpy(qfh.last_qhd1.lastoper, "---");
	strcpy(qfh.last_qhd2.lastoper, "---");

/*
"-U", "-G" が有効なのは CAP_CHOWN 権限を持つときのみ
*/

	b = sfq_pwdgrp_nam2id(qip->queusrnam, qip->quegrpnam, &queusrid, &quegrpid);
	if (! b)
	{
		SFQ_FAIL(EA_PWDNAME2ID, "pwd_nam2id");
	}

	if (sfq_caps_isset(CAP_CHOWN))
	{
		if (SFQ_ISSET_UID(queusrid))
		{
			if (SFQ_ISSET_GID(quegrpid))
			{
				/* chown queusrid.quegrpid */
				/* chmod g+w */

				chmod_GaW= SFQ_true;
			}
			else
			{
				/* chown queusrid.root */
			}
		}
		else if (SFQ_ISSET_GID(quegrpid))
		{
			/* chown root.quegrpid */
			/* chmod g+w */

			chmod_GaW= SFQ_true;
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
		if (SFQ_ISSET_UID(queusrid))
		{
			if (queusrid != euid)
			{
				SFQ_FAIL(EA_NOTPERMIT, "specified user, operation not permitted");
			}
		}

		if (SFQ_ISSET_GID(quegrpid))
		{
			if (quegrpid != egid)
			{
				SFQ_FAIL(EA_NOTPERMIT, "specified group, operation not permitted");
			}

			/* chmod g+w */

			chmod_GaW = SFQ_true;
		}

		queusrid = SFQ_UID_NONE;
		quegrpid = SFQ_GID_NONE;
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
			SFQ_FAIL(ES_MEMORY, "ALLOC(procs)");
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

		SFQ_FAIL(EA_FSIZESMALL, "specified filesize-limit too small (limit=%zu)",
			qip->filesize_limit);
	}

/* open queue-file */
	qcp.querootdir = querootdir;
	qcp.quename = quename;
	qcp.queusrid = queusrid;
	qcp.quegrpid = quegrpid;
	qcp.chmod_GaW = chmod_GaW;

	qo = sfq_open_queue_wo(&qcp);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "sfq_create_queue");
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
		SFQ_FAIL(EA_QFHRW, "sfq_writeqfh");
	}

/* print queue header */
	sfq_print_qo(qo);
	sfq_print_qf_header(&qfh);

SFQ_LIB_CHECKPOINT

	sfq_close_queue(qo);
	qo = NULL;

SFQ_ENTP_LEAVE

	return SFQ_LIB_RC();
}

