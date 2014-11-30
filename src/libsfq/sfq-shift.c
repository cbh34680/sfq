#include "sfq-lib.h"

int sfq_shift(const char* querootdir, const char* quename, struct sfq_value* val)
{
LIBFUNC_INITIALIZE

	struct sfq_queue_object* qo = NULL;

	bool b = false;
	size_t eh_size = 0;
	off_t elm_pos = 0;

	struct sfq_ioelm_buff ioeb;
	struct sfq_file_header qfh;

/* initialize */
	eh_size = sizeof(struct sfq_e_header);

	bzero(&qfh, sizeof(qfh));
	bzero(&ioeb, sizeof(ioeb));

/* */
	if (! val)
	{
		FIRE(SFQ_RC_EA_FUNCARG, "val is null");
	}

/* open queue-file */
	qo = sfq_open_queue(querootdir, quename, "rb+");
	if (! qo)
	{
		FIRE(SFQ_RC_EA_OPENFILE, "sfq_open_queue");
	}

/* read file-header */
	b = sfq_readqfh(qo->fp, &qfh, NULL);
	if (! b)
	{
		FIRE(SFQ_RC_EA_READQFH, "sfq_readqfh");
	}

	qfh.last_qhd2 = qfh.last_qhd1;
	qfh.last_qhd1 = qfh.qh.dval;

/* check empty */
	if (qfh.qh.dval.elm_num == 0)
	{
		FIRE(SFQ_RC_NO_ELEMENT, "no element/shift");
	}

	if (qfh.qh.dval.elm_next_shift_pos == 0)
	{
		FIRE(SFQ_RC_EA_ASSERT, "qfh.qh.dval.elm_next_shift_pos == 0");
	}

	elm_pos = qfh.qh.dval.elm_next_shift_pos;

/* read element */
	b = sfq_readelm(qo->fp, elm_pos, &ioeb);
	if (! b)
	{
		FIRE(SFQ_RC_EA_RWELEMENT, "sfq_readelm");
	}

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
		struct sfq_e_header next_eh;

		b = sfq_seek_set_and_read(qo->fp, ioeb.eh.next_elmpos, &next_eh, eh_size);
		if (! b)
		{
			FIRE(SFQ_RC_EA_SEEKSETIO, "sfq_seek_set_and_read(next_eh)");
		}

		/* 次の要素の prev_elmpos に 0 を設定し、リンクを切る */
		next_eh.prev_elmpos = 0;

		b = sfq_seek_set_and_write(qo->fp, ioeb.eh.next_elmpos, &next_eh, eh_size);
		if (! b)
		{
			FIRE(SFQ_RC_EA_SEEKSETIO, "sfq_seek_set_and_write(next_eh)");
		}

/* update next shift pos */
		qfh.qh.dval.elm_next_shift_pos = ioeb.eh.next_elmpos;
	}

	strcpy(qfh.qh.dval.lastoper, "SHF");
	qfh.qh.dval.update_num++;
	qfh.qh.dval.updatetime = qo->opentime;

/* overwrite header */
	b = sfq_seek_set_and_write(qo->fp, 0, &qfh, sizeof(qfh));
	if (! b)
	{
		FIRE(SFQ_RC_EA_SEEKSETIO, "sfq_seek_set_and_write(qfh)");
	}

#ifdef SFQ_DEBUG_BUILD
	sfq_print_q_header(&qfh.qh);
#endif

/* set val */
	b = sfq_copy_ioeb2val(&ioeb, val);
	if (! b)
	{
		FIRE(SFQ_RC_EA_COPYVALUE, "sfq_copy_ioeb2val");
	}

LIBFUNC_COMMIT

	sfq_close_queue(qo);
	qo = NULL;

	if (LIBFUNC_IS_ROLLBACK())
	{
		sfq_free_ioelm_buff(&ioeb);
	}

LIBFUNC_FINALIZE

	return LIBFUNC_RC();
}

