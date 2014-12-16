#include "sfq-lib.h"

int sfq_map(const char* querootdir, const char* quename, sfq_map_callback callback, void* userdata,
	sfq_bool reverse, ulong loop_limit)
{
SFQ_ENTP_ENTER

	struct sfq_queue_object* qo = NULL;

	ulong num = 0;
	sfq_bool b = SFQ_false;
	off_t elm_pos = 0;

	struct sfq_file_header qfh;
	struct sfq_ioelm_buff ioeb;

/* initialize */
	bzero(&qfh, sizeof(qfh));
	bzero(&ioeb, sizeof(ioeb));

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

#ifdef SFQ_DEBUG_BUILD
	assert(qfh.qh.dval.elm_next_shift_pos);
#endif
	if (qfh.qh.dval.elm_next_shift_pos == 0)
	{
		SFQ_FAIL(EA_ASSERT, "qfh.qh.dval.elm_next_shift_pos == 0");
	}

/* loop elements */
	elm_pos = reverse
		? qfh.qh.dval.elm_last_push_pos
		: qfh.qh.dval.elm_next_shift_pos;

	//for (num=0, elm_pos=qfh.qh.dval.elm_next_shift_pos; elm_pos; num++)
	for (num=0; elm_pos; num++)
	{
		struct sfq_value val;

		if (loop_limit)
		{
			if (num >= loop_limit)
			{
				break;
			}
		}

		bzero(&val, sizeof(val));

		b = sfq_readelm(qo, elm_pos, &ioeb);
		if (! b)
		{
			SFQ_FAIL(EA_RWELEMENT, "sfq_readelm");
		}

#ifdef SFQ_DEBUG_BUILD
		sfq_print_e_header(&ioeb.eh);
#else
		if (loop_limit)
		{
			sfq_print_e_header(&ioeb.eh);
		}
#endif

/* set val */
		b = sfq_copy_ioeb2val(&ioeb, &val);
		if (! b)
		{
			SFQ_FAIL(EA_COPYVALUE, "sfq_copy_ioeb2val");
		}

		callback(num, elm_pos, &val, userdata);

/* copy next element-offset to pos */
		//elm_pos = ioeb.eh.next_elmpos;
		elm_pos = reverse
			? ioeb.eh.prev_elmpos
			: ioeb.eh.next_elmpos;

		sfq_free_ioelm_buff(&ioeb);
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

