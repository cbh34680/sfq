#include "sfq-lib.h"

int sfq_info(const char* querootdir, const char* quename)
{
SFQ_LIB_INITIALIZE

	struct sfq_queue_object* qo = NULL;
	struct sfq_process_info* procs = NULL;

	bool b = false;

	struct sfq_file_header qfh;

/* initialize */
	bzero(&qfh, sizeof(qfh));

/* open queue-file */
	qo = sfq_open_queue_ro(querootdir, quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENFILE, "sfq_open_queue");
	}

	sfq_print_sizes();

/* read file-header */
	b = sfq_readqfh(qo, &qfh, &procs);
	if (! b)
	{
		SFQ_FAIL(EA_READQFH, "sfq_readqfh");
	}

/* print queue header */
	sfq_print_qf_header(&qfh);

	if (procs)
	{
		sfq_print_procs(procs, qfh.qh.sval.procs_num);
	}

SFQ_LIB_CHECKPOINT

	free(procs);
	procs = NULL;

SFQ_LIB_FINALIZE

	sfq_close_queue(qo);
	qo = NULL;

	return SFQ_LIB_RC();
}

