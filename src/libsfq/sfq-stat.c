#include "sfq-lib.h"

int sfq_set_header_by_name(const char* querootdir, const char* quename,
        const char* member_name, const void* data, size_t data_size, int semlock_wait_sec)
{
	struct member_size_map
	{
		const char* name;
		size_t data_size;
	}
	ms_map[] =
	{
		{ "payloadsize_limit",	sizeof(ulong) },
		{ "filesize_limit",	sizeof(ulong) },
		{ "execable_maxla",	sizeof(ushort) },
		{ "execloop_sleep",	sizeof(sfq_uchar) },
		{ NULL,			0 },
	};

SFQ_LIB_ENTER

	struct sfq_queue_object* qo = NULL;

	struct sfq_file_header qfh;
	sfq_bool b = SFQ_false;

	struct member_size_map* ms_set = NULL;
	void* addr = NULL;

/* initialize */
	bzero(&qfh, sizeof(qfh));

	if (! member_name)
	{
		SFQ_FAIL(EA_FUNCARG, "member_name not set");
	}
	if (! data)
	{
		SFQ_FAIL(EA_FUNCARG, "data not set");
	}
	if (! data_size)
	{
		SFQ_FAIL(EA_FUNCARG, "data_size not set");
	}

	for (ms_set=ms_map; ms_set->name; ms_set++)
	{
		if (strcmp(member_name, ms_set->name) == 0)
		{
			if (ms_set->data_size != data_size)
			{
				SFQ_FAIL(EA_FUNCARG, "data_size is not match (%zu:%zu)",
					ms_set->data_size, data_size);
			}
			break;
		}
	}

	if (! ms_set->name)
	{
		SFQ_FAIL(EA_NOTMEMBER, "member(%s) not exist", member_name);
	}

	qo = sfq_open_queue_rw_lws(querootdir, quename, semlock_wait_sec);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "sfq_open_queue_ro_lws");
	}

	b = sfq_readqfh(qo, &qfh, NULL);
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "sfq_readqfh");
	}

	if (strcmp(ms_set->name, "payloadsize_limit") == 0)
	{
		ulong ul = *((ulong*)data);

		if (ul == qfh.qh.sval.payloadsize_limit)
		{
			SFQ_FAIL(EA_FUNCARG, "no change");
		}

		addr = &qfh.qh.sval.payloadsize_limit;
	}
	else if (strcmp(ms_set->name, "filesize_limit") == 0)
	{
		ulong ul = *((ulong*)data);

		if (ul == qfh.qh.sval.filesize_limit)
		{
			SFQ_FAIL(EA_FUNCARG, "no change");
		}
		else if (ul > qfh.qh.sval.filesize_limit)
		{
		}
		else /* ul < filesize_limit */
		{
/*
現在のサイズより小さくなる場合
*/
			if (qfh.qh.dval.elm_next_push_pos == qfh.qh.dval.elm_next_shift_pos)
			{
/*
要素 0
*/
				assert(qfh.qh.dval.elm_num == 0);
			}
			else if (qfh.qh.dval.elm_next_shift_pos < qfh.qh.dval.elm_next_push_pos)
			{
/*
shift 位置 < push 位置 の場合 ... 通常
*/
				if (ul < qfh.qh.dval.elm_next_push_pos)
				{
					SFQ_FAIL(EA_CHANGESIZE, "element is at the end of the file/1");
				}
			}
			else
			{
/*
push 位置 < shift 位置 の場合 ... 循環済
*/
				SFQ_FAIL(EA_CHANGESIZE, "element is at the end of the file/2");
			}

			size_t smallest_size = qfh.qh.sval.elmseg_start_pos + sfq_get_smallest_elmsize();

			if (ul < smallest_size)
			{
				SFQ_FAIL(EA_FSIZESMALL, "filesize-limit too small (specified=%zu) (minimum=%zu)",
					ul, smallest_size);
			}
		}

		qfh.qh.sval.elmseg_end_pos = ul - 1u;
		addr = &qfh.qh.sval.filesize_limit;
	}
	else if (strcmp(ms_set->name, "execable_maxla") == 0)
	{
		addr = &qfh.qh.sval.execable_maxla;
	}
	else if (strcmp(ms_set->name, "execloop_sleep") == 0)
	{
		addr = &qfh.qh.sval.execloop_sleep;
	}

	if (! addr)
	{
		SFQ_FAIL(EA_ASSERT, "addr is not set");
	}

	memcpy(addr, data, data_size);

	b = sfq_writeqfh(qo, &qfh, NULL, "SET");
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "sfq_writeqfh");
	}

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	sfq_close_queue(qo);
	qo = NULL;

	return SFQ_LIB_RC();
}

int sfq_get_questate(const char* querootdir, const char* quename,
	questate_t* questate_ptr, int semlock_wait_sec)
{
SFQ_LIB_ENTER

	struct sfq_queue_object* qo = NULL;

	struct sfq_file_header qfh;
	sfq_bool b = SFQ_false;

/* initialize */
	bzero(&qfh, sizeof(qfh));

	if (! questate_ptr)
	{
		SFQ_FAIL(EA_FUNCARG, "questate_ptr is null");
	}

	qo = sfq_open_queue_ro_lws(querootdir, quename, semlock_wait_sec);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "sfq_open_queue_ro_lws");
	}

	b = sfq_readqfh(qo, &qfh, NULL);
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "sfq_readqfh");
	}

	(*questate_ptr) = qfh.qh.dval.questate;

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	sfq_close_queue(qo);
	qo = NULL;

	return SFQ_LIB_RC();
}

int sfq_set_questate(const char* querootdir, const char* quename,
	questate_t questate, int semlock_wait_sec)
{
SFQ_LIB_ENTER

	struct sfq_open_names* om = NULL;
	struct sfq_queue_object* qo = NULL;
	struct sfq_process_info* procs = NULL;

	int irc = -1;
	int slotno = -1;
	sfq_bool b = SFQ_false;

	sfq_bool forceLeakQueue = SFQ_false;

	struct sfq_file_header qfh;

/* initialize */
	bzero(&qfh, sizeof(qfh));

/* create names */
	if (questate & SFQ_QST_DEV_SEMUNLOCK_ON)
	{
/*
!! デバッグ用 !!

強制的にセマフォのロックを解除する
*/
		om = sfq_alloc_open_names(querootdir, quename);
		if (! om)
		{
			SFQ_FAIL(EA_CREATENAMES, "sfq_alloc_open_names");
		}

		irc = sem_unlink(om->semname);
		if (irc == -1)
		{
			if (errno != ENOENT)
			{
				SFQ_FAIL(ES_FILE,
					"delete semaphore fault, check permission (e.g. /dev/shm%s)",
					om->semname);
			}
		}

		SFQ_FAIL(ES_SEMAPHORE, "success unlock semaphore [%s] (for develop)\n", om->semname);
	}

/* open queue */
	qo = sfq_open_queue_rw_lws(querootdir, quename, semlock_wait_sec);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "sfq_open_queue_rw");
	}

/* */
	if (questate & SFQ_QST_DEV_SEMLOCK_ON)
	{
/*
!! デバッグ用 !!

セマフォをロッセしたままにする
--> メモリリークは発生する
*/
		forceLeakQueue = SFQ_true;

		SFQ_FAIL(ES_SEMAPHORE, "success lock semaphore [%s] (for develop)\n", qo->om->semname);
	}

/* read queue header */
	b = sfq_readqfh(qo, &qfh, &procs);
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "sfq_readqfh");
	}

/* update state */
	if (qfh.qh.dval.questate == questate)
	{
/*
変更前後が同じ値なら更新の必要はない
*/
		SFQ_FAIL(W_NOCHANGE_STATE, "there is no change in the state");
	}

	if (questate & SFQ_QST_EXEC_ON)
	{
/*
exec が OFF から ON に変わった
*/
		if (qfh.qh.dval.elm_num)
		{
/*
キューに要素が存在する
*/
			if (procs)
			{
/*
プロセステーブルが存在するので、実行予約を試みる
*/
				slotno = sfq_reserve_proc(procs, qfh.qh.sval.procs_num);
			}
		}
	}

	if (slotno == -1)
	{
/*
プロセステーブルが更新されていないので開放
*/
		free(procs);
		procs = NULL;
	}

	qfh.qh.dval.questate = questate;

	b = sfq_writeqfh(qo, &qfh, procs, "SET");
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "sfq_writeqfh");
	}

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	sfq_free_open_names(om);
	om = NULL;

	free(procs);
	procs = NULL;

	if (forceLeakQueue)
	{
		/* for Debug */
	}
	else
	{
		sfq_close_queue(qo);
		qo = NULL;
	}

	if (slotno != -1)
	{
		sfq_go_exec(querootdir, quename, (ushort)slotno, questate);
	}

	return SFQ_LIB_RC();
}

