#include "sfq-lib.h"

int sfq_clear(const char* querootdir, const char* quename)
{
SFQ_LIB_INITIALIZE

	struct sfq_queue_object* qo = NULL;

	bool b = false;
	struct sfq_file_header qfh;

/* initialize */

	bzero(&qfh, sizeof(qfh));

/* open queue-file */
	qo = sfq_open_queue(querootdir, quename, "rb+");
	if (! qo)
	{
		SFQ_FAIL(EA_OPENFILE, "sfq_open_queue");
	}

/* read file-header */
	b = sfq_readqfh(qo->fp, &qfh, NULL);
	if (! b)
	{
		SFQ_FAIL(EA_READQFH, "sfq_readqfh");
	}

	qfh.last_qhd2 = qfh.last_qhd1;
	qfh.last_qhd1 = qfh.qh.dval;

	sfq_qh_init_pos(&qfh.qh);

	strcpy(qfh.qh.dval.lastoper, "CLR");
	qfh.qh.dval.update_num++;
	qfh.qh.dval.updatetime = qo->opentime;

/* overwrite header */
	b = sfq_seek_set_and_write(qo->fp, 0, &qfh, sizeof(qfh));
	if (! b)
	{
		SFQ_FAIL(EA_SEEKSETIO, "sfq_seek_set_and_write(qfh)");
	}

#ifdef SFQ_DEBUG_BUILD
	sfq_print_q_header(&qfh.qh);
#endif

SFQ_LIB_CHECKPOINT

	sfq_close_queue(qo);
	qo = NULL;

SFQ_LIB_FINALIZE

	return SFQ_LIB_RC();
}

