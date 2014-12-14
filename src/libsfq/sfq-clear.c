#include "sfq-lib.h"

int sfq_clear(const char* querootdir, const char* quename)
{
SFQ_ENTP_INITIALIZE

	struct sfq_queue_object* qo = NULL;

	bool b = false;
	struct sfq_file_header qfh;

/* initialize */

	errno = 0;
	bzero(&qfh, sizeof(qfh));

/* open queue-file */
	qo = sfq_open_queue_rw(querootdir, quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENFILE, "sfq_open_queue");
	}

/* read file-header */
	b = sfq_readqfh(qo, &qfh, NULL);
	if (! b)
	{
		SFQ_FAIL(EA_READQFH, "sfq_readqfh");
	}

	sfq_qh_init_pos(&qfh.qh);

/* overwrite header */
	b = sfq_writeqfh(qo, &qfh, NULL, "CLR");
	if (! b)
	{
		SFQ_FAIL(EA_WRITEQFH, "sfq_writeqfh");
	}

#ifdef SFQ_DEBUG_BUILD
	sfq_print_q_header(&qfh.qh);
#endif

SFQ_LIB_CHECKPOINT

SFQ_ENTP_FINALIZE

	sfq_close_queue(qo);
	qo = NULL;

	return SFQ_LIB_RC();
}

