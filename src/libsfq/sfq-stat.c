#include "sfq-lib.h"

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
		SFQ_FAIL(EA_OPENQUEUE, "sfq_open_queue_ro");
	}

	b = sfq_readqfh(qo, &qfh, NULL);
	if (! b)
	{
		SFQ_FAIL(EA_READQFH, "sfq_readqfh");
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
				SFQ_FAIL(ES_UNLINK,
					"delete semaphore fault, check permission (e.g. /dev/shm%s)",
					om->semname);
			}
		}

		SFQ_FAIL(DEV_SEMUNLOCK, "success unlock semaphore [%s] (for develop)\n", om->semname);
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

		SFQ_FAIL(DEV_SEMLOCK, "success lock semaphore [%s] (for develop)\n", qo->om->semname);
	}

/* read queue header */
	b = sfq_readqfh(qo, &qfh, &procs);
	if (! b)
	{
		SFQ_FAIL(EA_READQFH, "sfq_readqfh");
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
		SFQ_FAIL(EA_WRITEQFH, "sfq_writeqfh");
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

