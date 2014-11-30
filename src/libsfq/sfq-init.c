#include "sfq-lib.h"

int sfq_init(const char* querootdir, const char* quename,
	size_t filesize_limit, size_t payloadsize_limit, ushort max_proc_num)
{
SFQ_LIB_INITIALIZE

	struct sfq_queue_object* qo = NULL;
	struct sfq_process_info* procs = NULL;

	size_t iosize = 0;
	ushort qfh_size = 0;
	size_t eh_size = 0;
	size_t pi_size = 0;
	size_t procs_size = 0;
	size_t elmseg_start_pos = 0;

	struct sfq_file_header qfh;

/* initialize */
	qfh_size = sizeof(qfh);
	eh_size = sizeof(struct sfq_e_header);
	pi_size = sizeof(struct sfq_process_info);

	bzero(&qfh, qfh_size);

/* check max process num */

	if (max_proc_num)
	{
		long sysmax = sysconf(_SC_CHILD_MAX);
		if (sysmax > 0)
		{
			if (max_proc_num > sysmax)
			{
				SFQ_FAIL(EA_OVERLIMIT, "max_proc_num");
			}
		}

/* calc procs-start offset */
		procs_size = pi_size * max_proc_num;

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
	if (filesize_limit < (elmseg_start_pos + eh_size + 1))
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

/* initialize queue-header */

	/* 静的属性値の設定 */
	strcpy(qfh.magicstr, SFQ_MAGICSTR);
	qfh.qfh_size = qfh_size;

	qfh.qh.sval.elmseg_start_pos = elmseg_start_pos;
	qfh.qh.sval.elmseg_end_pos = (filesize_limit - 1);
	qfh.qh.sval.filesize_limit = filesize_limit;
	qfh.qh.sval.payloadsize_limit = payloadsize_limit;
	qfh.qh.sval.max_proc_num = max_proc_num;

	/* 動的属性値の初期化 */
	strcpy(qfh.qh.dval.lastoper, "INI");
	strcpy(qfh.last_qhd1.lastoper, "---");
	strcpy(qfh.last_qhd2.lastoper, "---");

	/* 各ポジションの初期化 */
/*
静的属性値の値を使っているので sfq_qh_init_pos() の位置は動かさないこと
*/
	sfq_qh_init_pos(&qfh.qh);

/* write queue-header */
	/* w: sfq_file_header */
	iosize = fwrite(&qfh, qfh_size, 1, qo->fp);
	if (iosize != 1)
	{
		SFQ_FAIL(ES_FILEIO, "FILE-WRITE(qfh)");
	}

	if (procs_size)
	{
	/* w: pid_table */
		iosize = fwrite(procs, procs_size, 1, qo->fp);
		if (iosize != 1)
		{
			SFQ_FAIL(ES_FILEIO, "FILE-WRITE(procs)");
		}
	}

#ifdef SFQ_DEBUG_BUILD
	sfq_print_qf_header(&qfh);
#endif

SFQ_LIB_CHECKPOINT

	sfq_close_queue(qo);
	qo = NULL;

SFQ_LIB_FINALIZE

	return SFQ_LIB_RC();
}

