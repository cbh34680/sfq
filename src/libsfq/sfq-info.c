#include "sfq-lib.h"

int sfq_info(const char* querootdir, const char* quename, int semlock_wait_sec)
{
SFQ_ENTP_ENTER

	struct sfq_queue_object* qo = NULL;
	struct sfq_process_info* procs = NULL;

	sfq_bool b = SFQ_false;

	struct sfq_file_header qfh;

/* initialize */
	bzero(&qfh, sizeof(qfh));

/* open queue-file */
	qo = sfq_open_queue_ro(querootdir, quename, semlock_wait_sec);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "sfq_open_queue_ro");
	}

//#ifdef SFQ_DEBUG_BUILD
//	sfq_print_sizes();
//#endif

/* read file-header */
	b = sfq_readqfh(qo, &qfh, &procs);
	if (! b)
	{
		SFQ_FAIL(EA_READQFH, "sfq_readqfh");
	}

/* print queue header */
	sfq_print_qo(qo);
	sfq_print_qf_header(&qfh);
	sfq_print_procs(procs, qfh.qh.sval.procs_num);

SFQ_LIB_CHECKPOINT

	free(procs);
	procs = NULL;

	sfq_close_queue(qo);
	qo = NULL;

SFQ_ENTP_LEAVE

	return SFQ_LIB_RC();
}

