#include "sfq-lib.h"

int sfq_info(const char* querootdir, const char* quename)
{
SFQ_ENTP_INITIALIZE

	struct sfq_queue_object* qo = NULL;
	struct sfq_process_info* procs = NULL;

	bool b = false;

	struct sfq_file_header qfh;

/* initialize */
	errno = 0;
	bzero(&qfh, sizeof(qfh));

/* open queue-file */
	qo = sfq_open_queue_ro(querootdir, quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENFILE, "sfq_open_queue");
	}

#ifdef SFQ_DEBUG_BUILD
	//sfq_print_sizes();
#endif

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

SFQ_ENTP_FINALIZE

	sfq_close_queue(qo);
	qo = NULL;

	return SFQ_LIB_RC();
}

