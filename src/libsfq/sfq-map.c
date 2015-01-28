#include "sfq-lib.h"

static int sfq_map_(sfq_bool qopen_RW, const char* querootdir, const char* quename,
	sfq_map_callback callback, sfq_bool reverse, void* userdata)
{
SFQ_ENTP_ENTER

	struct sfq_queue_object* qo = NULL;

	ulong order = 1;
	sfq_bool b = SFQ_false;

	off_t endpos = 0;
	off_t currpos = 0;
	off_t savepos = 0;

	struct sfq_file_header qfh;
	struct sfq_ioelm_buff ioeb;

	sfq_bool loop_next = SFQ_true;

/* initialize */
	bzero(&qfh, sizeof(qfh));
	sfq_init_ioeb(&ioeb);

/* open queue-file */
	if (qopen_RW)
	{
		qo = sfq_open_queue_rw(querootdir, quename);
	}
	else
	{
		qo = sfq_open_queue_ro(querootdir, quename);
	}

	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "sfq_open_queue_ro");
	}

/* read file-header */
	b = sfq_readqfh(qo, &qfh, NULL);
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "sfq_readqfh");
	}

/* check empty */
	if (qfh.qh.dval.elm_num == 0)
	{
		SFQ_FAIL_SILENT(W_NOELEMENT);
	}

	assert(qfh.qh.dval.elm_next_shift_pos);

/* loop elements */
	currpos = reverse
		? qfh.qh.dval.elm_next_pop_pos
		: qfh.qh.dval.elm_next_shift_pos;

	endpos = reverse
		? qfh.qh.dval.elm_next_shift_pos
		: qfh.qh.dval.elm_next_pop_pos;

/*
念のため currpos と savepos の両方を脱出条件としておく

--> currpos はシグナルでの強制終了時に不正な値となることが考えられるのであてにしないこと
--> currpos を条件から外した
*/
	while (/*currpos && */ (savepos != endpos) && loop_next)
	{
		struct sfq_value val;
		struct sfq_map_callback_param cb_param;

/* */
		bzero(&val, sizeof(val));
		bzero(&cb_param, sizeof(cb_param));

		savepos = currpos;

/* */
		b = sfq_readelm_alloc(qo, currpos, &ioeb);
		if (! b)
		{
			SFQ_FAIL(EA_ELMRW, "sfq_readelm_alloc");
		}

/* set val */
		b = sfq_copy_ioeb2val(&ioeb, &val);
		if (! b)
		{
			SFQ_FAIL(EA_COPYVALUE, "sfq_copy_ioeb2val");
		}

		val.querootdir = qo->om->querootdir;
		val.quename = qo->om->quename;

/*
パラメータを構築し、コールバック関数を呼び出す
*/
		cb_param.order = order;
		cb_param.elm_pos = currpos;
		cb_param.val = &val;
		cb_param.userdata = userdata;
		cb_param.disabled = ioeb.eh.disabled;

		loop_next = callback(&cb_param);
		order++;

/*
値が変更されていたら要素を更新する
*/
		if (ioeb.eh.disabled != cb_param.disabled)
		{
			b = sfq_disable_elm(qo, currpos, cb_param.disabled);
			if (! b)
			{
				SFQ_FAIL(EA_DISABLEELM, "sfq_disable_elm");
			}
		}

/* copy next element-offset to pos */
		currpos = reverse
			? ioeb.eh.prev_elmpos
			: ioeb.eh.next_elmpos;

		sfq_free_ioelm_buff(&ioeb);
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

int sfq_map_ro(const char* querootdir, const char* quename, sfq_map_callback callback,
	sfq_bool reverse, void* userdata)
{
	return sfq_map_(SFQ_false, querootdir, quename, callback, reverse, userdata);
}

int sfq_map_rw(const char* querootdir, const char* quename, sfq_map_callback callback,
	sfq_bool reverse, void* userdata)
{
	return sfq_map_(SFQ_true, querootdir, quename, callback, reverse, userdata);
}

