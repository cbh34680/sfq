#include "sfq-lib.h"

int sfq_clear(const char* querootdir, const char* quename)
{
SFQ_ENTP_ENTER

	struct sfq_queue_object* qo = NULL;

	sfq_bool b = SFQ_false;
	struct sfq_file_header qfh;

/* initialize */

	bzero(&qfh, sizeof(qfh));

/* open queue-file */
	qo = sfq_open_queue_rw(querootdir, quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "sfq_open_queue_rw");
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

	sfq_close_queue(qo);
	qo = NULL;

SFQ_ENTP_LEAVE

	return SFQ_LIB_RC();
}

