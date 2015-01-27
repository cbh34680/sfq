#include "sfq-lib.h"

int sfq_pop(const char* in_querootdir, const char* in_quename, struct sfq_value* val)
{
SFQ_ENTP_ENTER

	struct sfq_queue_object* qo = NULL;
	const char* out_querootdir = NULL;
	const char* out_quename = NULL;

	sfq_bool b = SFQ_false;
	off_t elmpos = 0;
	off_t* unlink_pos = NULL;

	struct sfq_ioelm_buff ioeb;
	struct sfq_file_header qfh;

/* initialize */
	bzero(&qfh, sizeof(qfh));
	sfq_init_ioeb(&ioeb);

/* check argument */
	if (! val)
	{
		SFQ_FAIL(EA_FUNCARG, "val is null");
	}

/* open queue-file */
	qo = sfq_open_queue_rw(in_querootdir, in_quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "sfq_open_queue_rw");
	}

	out_querootdir = strdup(qo->om->querootdir);
	if (! out_querootdir)
	{
		SFQ_FAIL(ES_MEMORY, "out_querootdir");
	}

	out_quename = strdup(qo->om->quename);
	if (! out_quename)
	{
		SFQ_FAIL(ES_MEMORY, "out_quename");
	}

/* read file-header */
	b = sfq_readqfh(qo, &qfh, NULL);
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "sfq_readqfh");
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
	b = sfq_readelm_alloc(qo, elmpos, &ioeb);
	if (! b)
	{
		SFQ_FAIL(EA_ELMRW, "sfq_readelm_alloc");
	}

//#ifdef SFQ_DEBUG_BUILD
//	sfq_print_e_header(&ioeb.eh);
//#endif

/* update queue file header */

	/* 要素数を減算 */
	qfh.qh.dval.elm_num--;

	/* 要素サイズを減算 */
	qfh.qh.dval.elmsize_total_ -= ioeb.eh.elmsize_;

/* */
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
		SFQ_FAIL(EA_QFHRW, "sfq_writeqfh(qfh)");
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

	val->querootdir = out_querootdir;
	val->quename = out_quename;

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		sfq_free_ioelm_buff(&ioeb);

		free((char*)out_querootdir);
		out_querootdir = NULL;

		free((char*)out_quename);
		out_quename = NULL;
	}

	sfq_close_queue(qo);
	qo = NULL;

SFQ_ENTP_LEAVE

	return SFQ_LIB_RC();
}

