#include "sfq-lib.h"

int sfq_map(const char* querootdir, const char* quename, sfq_map_callback callback,
	sfq_bool reserve, void* userdata)
{
SFQ_ENTP_ENTER

	struct sfq_queue_object* qo = NULL;

	ulong num = 0;
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
	qo = sfq_open_queue_ro(querootdir, quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "sfq_open_queue_ro");
	}

/* read file-header */
	b = sfq_readqfh(qo, &qfh, NULL);
	if (! b)
	{
		SFQ_FAIL(EA_READQFH, "sfq_readqfh");
	}

/* check empty */
	if (qfh.qh.dval.elm_num == 0)
	{
		SFQ_FAIL_SILENT(W_NOELEMENT);
	}

	assert(qfh.qh.dval.elm_next_shift_pos);

/* loop elements */
	currpos = reserve
		? qfh.qh.dval.elm_next_pop_pos
		: qfh.qh.dval.elm_next_shift_pos;

	endpos = reserve
		? qfh.qh.dval.elm_next_shift_pos
		: qfh.qh.dval.elm_next_pop_pos;

/*
念のため currpos と savepos の両方を脱出条件としておく

--> currpos はシグナルでの強制終了時に不正な値となることが考えられるのであてにしないこと
--> currpos を条件から外した
*/
	for (num=0; /*currpos && */(savepos != endpos) && loop_next; num++)
	{
		struct sfq_value val;

/* */
		savepos = currpos;
		bzero(&val, sizeof(val));

		b = sfq_readelm_alloc(qo, currpos, &ioeb);
		if (! b)
		{
			SFQ_FAIL(EA_RWELEMENT, "sfq_readelm_alloc");
		}

#ifdef SFQ_DEBUG_BUILD
		sfq_print_e_header(&ioeb.eh);
#endif

/* set val */
		b = sfq_copy_ioeb2val(&ioeb, &val);
		if (! b)
		{
			SFQ_FAIL(EA_COPYVALUE, "sfq_copy_ioeb2val");
		}

		val.querootdir = qo->om->querootdir;
		val.quename = qo->om->quename;

		loop_next = callback(num, currpos, &val, userdata);

/* copy next element-offset to pos */
		currpos = reserve
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

