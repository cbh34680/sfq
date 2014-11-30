#include "sfq-lib.h"

int sfq_info(const char* querootdir, const char* quename)
{
LIBFUNC_INITIALIZE

	struct sfq_queue_object* qo = NULL;
	struct sfq_process_info* procs = NULL;

	bool b = false;

	struct sfq_file_header qfh;

/* initialize */
	bzero(&qfh, sizeof(qfh));

/* open queue-file */
	qo = sfq_open_queue(querootdir, quename, "rb");
	if (! qo)
	{
		FIRE(SFQ_RC_EA_OPENFILE, "sfq_open_queue");
	}

	sfq_print_sizes();

/* read file-header */
	b = sfq_readqfh(qo->fp, &qfh, &procs);
	if (! b)
	{
		FIRE(SFQ_RC_EA_READQFH, "sfq_readqfh");
	}

#ifndef SFQ_DEBUG_BUILD
/*
デバッグのときは sfq_readqfh() で print しているので、そうでないときだけ実行する
*/
	sfq_print_qf_header(&qfh);

	if (procs)
	{
		sfq_print_procs(procs, qfh.qh.sval.max_proc_num);
	}
#endif

LIBFUNC_COMMIT

	sfq_close_queue(qo);
	qo = NULL;

	free(procs);
	procs = NULL;

LIBFUNC_FINALIZE

	return LIBFUNC_RC();
}

