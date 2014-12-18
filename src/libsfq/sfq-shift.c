#include "sfq-lib.h"

int sfq_shift(const char* querootdir, const char* quename, struct sfq_value* val)
{
SFQ_ENTP_ENTER

	struct sfq_queue_object* qo = NULL;

	sfq_bool b = SFQ_false;
#if 0
	size_t eh_size = 0;
#endif
	off_t elm_pos = 0;

	struct sfq_ioelm_buff ioeb;
	struct sfq_file_header qfh;

/* initialize */
#if 0
	eh_size = sizeof(struct sfq_e_header);
#endif

	bzero(&qfh, sizeof(qfh));
	bzero(&ioeb, sizeof(ioeb));

/* */
	if (! val)
	{
		SFQ_FAIL(EA_FUNCARG, "val is null");
	}

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

#ifdef SFQ_DEBUG_BUILD
        assert(qfh.qh.dval.elm_next_shift_pos);
#endif
	if (qfh.qh.dval.elm_next_shift_pos == 0)
	{
		SFQ_FAIL(EA_ASSERT, "qfh.qh.dval.elm_next_shift_pos == 0");
	}

	elm_pos = qfh.qh.dval.elm_next_shift_pos;

/* read element */
	b = sfq_readelm(qo, elm_pos, &ioeb);
	if (! b)
	{
		SFQ_FAIL(EA_RWELEMENT, "sfq_readelm");
	}

#ifdef SFQ_DEBUG_BUILD
	sfq_print_e_header(&ioeb.eh);
#endif

/* update queue file header */
	qfh.qh.dval.elm_num--;

	if (qfh.qh.dval.elm_num == 0)
	{
	/* 要素がなくなったらヘッダのポジションを初期化する */

		sfq_qh_init_pos(&qfh.qh);
	}
	else
	{
/* update next-element-prev_elmpos */
#if 0
		struct sfq_e_header next_eh;

		b = sfq_seek_set_and_read(qo->fp, ioeb.eh.next_elmpos, &next_eh, eh_size);
		if (! b)
		{
			SFQ_FAIL(EA_SEEKSETIO, "sfq_seek_set_and_read(next_eh)");
		}

		/* 次の要素の prev_elmpos に 0 を設定し、リンクを切る */
		next_eh.prev_elmpos = 0;

		b = sfq_seek_set_and_write(qo->fp, ioeb.eh.next_elmpos, &next_eh, eh_size);
		if (! b)
		{
			SFQ_FAIL(EA_SEEKSETIO, "sfq_seek_set_and_write(next_eh)");
		}
#else
		b = sfq_unlink_prevelm(qo, ioeb.eh.next_elmpos);
		if (! b)
		{
			SFQ_FAIL(EA_UNLINKELM, "sfq_unlink_prevelm");
		}
#endif

/* update next shift pos */
		qfh.qh.dval.elm_next_shift_pos = ioeb.eh.next_elmpos;
	}

/* set val */
	b = sfq_copy_ioeb2val(&ioeb, val);
	if (! b)
	{
		SFQ_FAIL(EA_COPYVALUE, "sfq_copy_ioeb2val");
	}

/* update header */
	b = sfq_writeqfh(qo, &qfh, NULL, "SHF");
	if (! b)
	{
		SFQ_FAIL(EA_WRITEQFH, "sfq_writeqfh");
	}

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		sfq_free_ioelm_buff(&ioeb);
	}

SFQ_ENTP_LEAVE

	sfq_close_queue(qo);
	qo = NULL;

	return SFQ_LIB_RC();
}

