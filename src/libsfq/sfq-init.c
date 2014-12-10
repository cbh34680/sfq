#include "sfq-lib.h"

int sfq_init(const char* querootdir, const char* quename, const struct sfq_queue_create_option* qco)
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

	struct sfq_file_header qfh;

/* initialize */
	qfh_size = sizeof(qfh);
	eh_size = sizeof(struct sfq_e_header);
	pi_size = sizeof(struct sfq_process_info);

	bzero(&qfh, qfh_size);

/* */
	if (qco->procs_num < qco->boota_proc_num)
	{
		SFQ_FAIL(EA_OVERLIMIT, "bootable process must not be larger than process num");
	}

/*
queue header の初期値を設定
*/

	strcpy(qfh.magicstr, SFQ_MAGICSTR);
	qfh.qfh_size = qfh_size;
	qfh.qh.dval.questate = qco->questate;
	strcpy(qfh.last_qhd1.lastoper, "---");
	strcpy(qfh.last_qhd2.lastoper, "---");

/* check max process num */

	if (qco->procs_num)
	{
		long sysmax = sysconf(_SC_CHILD_MAX);
		if (sysmax > 0)
		{
			if (qco->procs_num > sysmax)
			{
				SFQ_FAIL(EA_OVERLIMIT, "process num is greater than system limit");
			}
		}

/* calc procs-start offset */
		procs_size = pi_size * qco->procs_num;

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
	if (qco->filesize_limit < (elmseg_start_pos + eh_size + 1))
	{
	/* 最小ファイルサイズは sfq_file_header + pid_table + sfq_e_header + payload(1 byte) */

		SFQ_FAIL(EA_FSIZESMALL, "specified filesize-limit too small");
	}

/* open queue-file */
	qo = sfq_create_queue(querootdir, quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENFILE, "sfq_create_queue");
	}

	if (procs)
	{
		if (qco->procs_num > qco->boota_proc_num)
		{
			int i = 0;

/*
起動可能数を超えたスロットはロックしておく

--> 配列の後方からロックする
*/
			for (i=0; i<(qco->procs_num - qco->boota_proc_num); i++)
			{
				struct sfq_process_info* proc = &procs[(qco->procs_num - 1) - i];

				proc->procstate = SFQ_PIS_LOCK;
				proc->updtime = qo->opentime;
			}
		}
	}

/* initialize queue-header */

	/* 静的属性値の設定 */
	qfh.qh.sval.elmseg_start_pos = elmseg_start_pos;
	qfh.qh.sval.elmseg_end_pos = (qco->filesize_limit - 1);
	qfh.qh.sval.filesize_limit = qco->filesize_limit;
	qfh.qh.sval.payloadsize_limit = qco->payloadsize_limit;
	qfh.qh.sval.procs_num = qco->procs_num;

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

