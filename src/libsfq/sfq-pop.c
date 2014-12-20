#include "sfq-lib.h"

int sfq_pop(const char* querootdir, const char* quename, struct sfq_value* val)
{
SFQ_ENTP_ENTER

	struct sfq_queue_object* qo = NULL;

	sfq_bool b = SFQ_false;
	off_t elmpos = 0;
	off_t* unlink_pos = NULL;

	struct sfq_ioelm_buff ioeb;
	struct sfq_file_header qfh;

/* initialize */
	bzero(&qfh, sizeof(qfh));
	bzero(&ioeb, sizeof(ioeb));

/* check argument */
	if (! val)
	{
		SFQ_FAIL(EA_FUNCARG, "val is null");
	}

/* open queue-file */
	qo = sfq_open_queue_rw(querootdir, quename, 0);
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

/* check accept state */
	if (! (qfh.qh.dval.questate & SFQ_QST_TAKEOUT_ON))
	{
		SFQ_FAIL_SILENT(W_TAKEOUT_STOPPED);
	}

/* check empty */
	if (qfh.qh.dval.elm_num == 0)
	{
		SFQ_FAIL_SILENT(W_NOELEMENT);
	}

//#ifdef SFQ_DEBUG_BUILD
//	assert(qfh.qh.dval.elm_next_shift_pos);
//#endif

	if (qfh.qh.dval.elm_next_pop_pos == 0)
	{
		SFQ_FAIL(EA_ASSERT, "qfh.qh.dval.elm_next_pop_pos == 0");
	}

	elmpos = qfh.qh.dval.elm_next_pop_pos;

/* read element */
	b = sfq_readelm(qo, elmpos, &ioeb);
	if (! b)
	{
		SFQ_FAIL(EA_RWELEMENT, "sfq_readelm");
	}

//#ifdef SFQ_DEBUG_BUILD
//	sfq_print_e_header(&ioeb.eh);
//#endif

/* update queue file header */
	qfh.qh.dval.elm_num--;

	if (qfh.qh.dval.elm_num == 0)
	{
	/* 要素がなくなったらヘッダのポジションを初期化する */

		sfq_qh_init_pos(&qfh.qh);
	}
	else
	{
		unlink_pos = &ioeb.eh.prev_elmpos;

/* update next shift pos */
		qfh.qh.dval.elm_next_pop_pos = ioeb.eh.prev_elmpos;

/*
pop したときは次回の push 位置も書き換える
*/
		qfh.qh.dval.elm_next_push_pos = elmpos;
	}

/* set val */
	b = sfq_copy_ioeb2val(&ioeb, val);
	if (! b)
	{
		SFQ_FAIL(EA_COPYVALUE, "sfq_copy_ioeb2val");
	}

/* update header */
	b = sfq_writeqfh(qo, &qfh, NULL, "POP");
	if (! b)
	{
		SFQ_FAIL(EA_WRITEQFH, "sfq_writeqfh(qfh)");
	}

	if (unlink_pos)
	{
/*
要素の next_elmpos を書き換えるが、writeqfh() より前に行うとシグナルにより
終了したときに回復できない状態になるので、ヘッダ書き換え後に行う。
*/
		b = sfq_unlink_nextelm(qo, (*unlink_pos));
		if (! b)
		{
			SFQ_FAIL(EA_UNLINKELM, "sfq_unlink_nextelm");
		}

		unlink_pos = NULL;
	}

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		sfq_free_ioelm_buff(&ioeb);
	}

	sfq_close_queue(qo);
	qo = NULL;

SFQ_ENTP_LEAVE

	return SFQ_LIB_RC();
}

