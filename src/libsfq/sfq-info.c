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
#ifdef SFQ_DEBUG_BUILD
	fprintf(stderr, "! om.querootdir:    %s\n",  qo->om->querootdir);
	fprintf(stderr, "! om.quename:       %s\n",  qo->om->quename);
	fprintf(stderr, "! om.quedir:        %s\n",  qo->om->quedir);
	fprintf(stderr, "! om.quefile:       %s\n",  qo->om->quefile);
	fprintf(stderr, "! om.quelogdir:     %s\n",  qo->om->quelogdir);
	fprintf(stderr, "! om.queproclogdir: %s\n",  qo->om->queproclogdir);
	fprintf(stderr, "! om.queexeclogdir: %s\n",  qo->om->queexeclogdir);
	fprintf(stderr, "! om.semname:       %s\n",  qo->om->semname);
	fprintf(stderr, "! om.opentime:      %zu\n", qo->opentime);
	fprintf(stderr, "\n");

	sfq_print_qf_header(&qfh);
#endif

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

