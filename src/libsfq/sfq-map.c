#include "sfq-lib.h"

int sfq_map(const char* querootdir, const char* quename, sfq_map_callback callback, void* userdata)
{
LIBFUNC_INITIALIZE

	struct sfq_queue_object* qo = NULL;

	ulong num = 0;
	bool b = false;
	off_t pos = 0;
	off_t elm_pos = 0;

	struct sfq_file_header qfh;
	struct sfq_ioelm_buff ioeb;

/* initialize */
	bzero(&qfh, sizeof(qfh));
	bzero(&ioeb, sizeof(ioeb));

/* open queue-file */
	qo = sfq_open_queue(querootdir, quename, "rb");
	if (! qo)
	{
		FIRE(SFQ_RC_EA_OPENFILE, "open_locked_file");
	}

/* read file-header */
	b = sfq_readqfh(qo->fp, &qfh, NULL);
	if (! b)
	{
		FIRE(SFQ_RC_EA_READQFH, "sfq_readqfh");
	}

/* check empty */
	if (qfh.qh.dval.elm_num == 0)
	{
		FIRE(SFQ_RC_NO_ELEMENT, "queue is empty");
	}

	if (qfh.qh.dval.elm_next_shift_pos == 0)
	{
		FIRE(SFQ_RC_EA_ASSERT, "qfh.qh.dval.elm_next_shift_pos == 0");
	}

	elm_pos = qfh.qh.dval.elm_next_shift_pos;

/* loop elements */
	for (num=0, pos=elm_pos; pos; num++)
	{
		struct sfq_value val;

		bzero(&val, sizeof(val));

		b = sfq_readelm(qo->fp, pos, &ioeb);
		if (! b)
		{
			FIRE(SFQ_RC_EA_RWELEMENT, "sfq_readelm");
		}

#ifdef SFQ_DEBUG_BUILD
		sfq_print_e_header(&ioeb.eh);

		fprintf(stderr, "* [seek-pos=%zu]\n", pos);
#endif

/* copy next element-offset to pos */
		pos = ioeb.eh.next_elmpos;

/* set val */
		b = sfq_copy_ioeb2val(&ioeb, &val);
		if (! b)
		{
			FIRE(SFQ_RC_EA_COPYVALUE, "sfq_copy_ioeb2val");
		}

		callback(num, &val, userdata);

		sfq_free_ioelm_buff(&ioeb);
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

