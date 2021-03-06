#include "sfq-lib.h"

#define STR_SET_NULL_IFEMPTY(a) \
	\
	if ( (a) ) \
	{ \
		if ( (a)[0] == '\0' ) \
		{ \
			(a) = NULL; \
		} \
	}

static int sfq_push_(struct sfq_queue_object* qo,
	struct sfq_value* val, short* slotno_ptr, questate_t* questate_ptr)
{
SFQ_LIB_ENTER

	struct sfq_process_info* procs = NULL;

	char* wrkdir = NULL;
	char* full_soutpath = NULL;
	char* full_serrpath = NULL;

	sfq_bool b = SFQ_false;

	int slotno = -1;
	off_t elm_pos = 0;
	questate_t questate = 0;
	int irc = -1;

	off_t IfPush_next_elmpos = 0;
	off_t IfPush_elm_end_pos = 0;

	struct sfq_file_header qfh;
	struct sfq_e_header prev_eh;
	struct sfq_ioelm_buff ioeb;

/* initialize */
	bzero(&qfh, sizeof(qfh));
	bzero(&prev_eh, sizeof(prev_eh));
	sfq_init_ioeb(&ioeb);

/* check argument */
	if (val && slotno_ptr && questate_ptr)
	{
		/* go next */
	}
	else
	{
		SFQ_FAIL(EA_FUNCARG, "args is null");
	}

/*
str[0] == '\0' のときは str に NULL を設定

--> 空文字列は存在しない状態にする
*/
	STR_SET_NULL_IFEMPTY(val->eworkdir);
	STR_SET_NULL_IFEMPTY(val->execpath);
	STR_SET_NULL_IFEMPTY(val->execargs);
	STR_SET_NULL_IFEMPTY(val->metatext);
	STR_SET_NULL_IFEMPTY(val->soutpath);
	STR_SET_NULL_IFEMPTY(val->serrpath);

/* get current directory */
	if (val->eworkdir)
	{
		struct stat stbuf;

		irc = stat(val->eworkdir, &stbuf);
		if (irc != 0)
		{
			SFQ_FAIL(EA_PATHNOTEXIST, "dir not exist '%s'", val->eworkdir);
		}

		if (! S_ISDIR(stbuf.st_mode))
		{
			SFQ_FAIL(EA_ISNOTDIR, "'%s' is not dir", val->eworkdir);
		}

		wrkdir = realpath(val->eworkdir, NULL);
		if (! wrkdir)
		{
			SFQ_FAIL(ES_MEMORY, "realpath(wrkdir)");
		}
	}

	if (! wrkdir)
	{
		wrkdir = getcwd(NULL, 0);
		if (! wrkdir)
		{
			SFQ_FAIL(ES_PATH, "getcwd");
		}
	}

	sfq_rtrim(wrkdir, "/");

	if (val->eworkdir)
	{
		val->eworkdir = wrkdir;
	}

/*
ログ関連は相対パスから絶対パスに変換
*/
	if (val->soutpath)
	{
		if ((val->soutpath[0] != '/') && (strcmp(val->soutpath, SFQ_DEFAULT_LOG) != 0))
		{
			full_soutpath = sfq_alloc_concat(wrkdir, "/", val->soutpath);
			if (! full_soutpath)
			{
				SFQ_FAIL(EA_CONCAT_N, "wrkdir/soutpath");
			}

			val->soutpath = full_soutpath;
		}
	}

	if (val->serrpath)
	{
		if ((val->serrpath[0] != '/') && (strcmp(val->serrpath, SFQ_DEFAULT_LOG) != 0))
		{
			full_serrpath = sfq_alloc_concat(wrkdir, "/", val->serrpath);
			if (! full_serrpath)
			{
				SFQ_FAIL(EA_CONCAT_N, "wrkdir/serrpath");
			}

			val->serrpath = full_serrpath;
		}
	}

/*
push 可能条件の判定
*/
	if ((! val->execpath) && (! val->execargs) && (! val->payload))
	{
		SFQ_FAIL(EA_FUNCARG, "no input data");
	}

/* copy arguments to buffer */
	b = sfq_copy_val2ioeb(val, &ioeb);
	if (! b)
	{
		SFQ_FAIL(EA_COPYVALUE, "sfq_copy_val2ioeb");
	}

/* read file-header */
	b = sfq_readqfh(qo, &qfh, &procs);
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "sfq_readqfh");
	}

/* check accept state */
	if (! (qfh.qh.dval.questate & SFQ_QST_ACCEPT_ON))
	{
		SFQ_FAIL_SILENT(W_ACCEPT_STOPPED);
	}

/*
questate は go_exec() に渡すので、ここで保存しておく
*/
	questate = qfh.qh.dval.questate;

/* check payload-size limit */
	if (qfh.qh.sval.payloadsize_limit)
	{
/*
# bug
#		if (val->payload_size > qfh.qh.sval.payloadsize_limit)
*/
		if (ioeb.eh.payload_size > qfh.qh.sval.payloadsize_limit)
		{
			SFQ_FAIL(EA_OVERLIMIT, "ioeb.eh.payload_size > qfh.qh.sval.payloadsize_limit");
		}
	}

	/* 最終 id を加算 */
	qfh.qh.dval.elm_lastid++;

/*
id, pushtime, uuid はここで生成する
*/
	ioeb.eh.id = qfh.qh.dval.elm_lastid;
	ioeb.eh.pushtime = qo->opentime;
	uuid_generate_random(ioeb.eh.uuid);

/* set cursor to pushable-pos */
	if (qfh.qh.dval.elm_next_shift_pos == qfh.qh.dval.elm_next_push_pos)
	{
	/* shift 位置 == push 位置 の場合 ... 全て shift している */

		assert(qfh.qh.dval.elm_num == 0);
/*
		if (qfh.qh.dval.elm_num != 0)
		{
			SFQ_FAIL(EA_ASSERT, "qfh.qh.dval.elm_num != 0");
		}
*/
		// GO NEXT
	}
	else if (qfh.qh.dval.elm_next_shift_pos < qfh.qh.dval.elm_next_push_pos)
	{
	/* shift 位置 < push 位置 の場合 ... 通常 */

		// GO NEXT
	}
	else
	{
	/* push 位置 < shift 位置 の場合 ... 循環済 */

		assert(qfh.qh.dval.elm_num);
/*
		if (qfh.qh.dval.elm_num == 0)
		{
			SFQ_FAIL(EA_ASSERT, "qfh.qh.dval.elm_num == 0");
		}
*/
		IfPush_next_elmpos = qfh.qh.dval.elm_next_push_pos + ioeb.eh.elmsize_;
		IfPush_elm_end_pos = IfPush_next_elmpos - 1;

		if (IfPush_elm_end_pos < (qfh.qh.dval.elm_next_shift_pos - 1))
		{
		/* 追加しても次回の shift 位置を越えない */

/*
本来はこの条件判定は IfPush_elm_end_pos < qfh.qh.dval.elm_next_shift_pos で良いのだが
そーすると "次回 push 位置" == "次回 shift 位置" になってしまい、その後が
面倒なので IfPush_elm_end_pos と qfh.qh.dval.elm_next_shift_pos の間に 1 byte 空ける
*/

			// GO NEXT
		}
		else
		{
			SFQ_FAIL_SILENT(W_NOSPACE);
		}
	}

	IfPush_next_elmpos = qfh.qh.dval.elm_next_push_pos + ioeb.eh.elmsize_;
	IfPush_elm_end_pos = IfPush_next_elmpos - 1;

	if (IfPush_elm_end_pos <= qfh.qh.sval.elmseg_end_pos)
	{
	/* 領域限界を超えない ... 通常 */

		/* push 位置へ移動 */
		elm_pos = qfh.qh.dval.elm_next_push_pos;

		/* 要素ヘッダを書き換え */
		ioeb.eh.prev_elmpos = qfh.qh.dval.elm_next_pop_pos;

		/* 属性ヘッダを書き換え */
		qfh.qh.dval.elm_next_pop_pos = qfh.qh.dval.elm_next_push_pos;
		qfh.qh.dval.elm_next_push_pos = IfPush_next_elmpos;
	}
	else
	{
	/* 領域限界を超える */

		IfPush_next_elmpos = qfh.qh.sval.elmseg_start_pos + ioeb.eh.elmsize_;
		IfPush_elm_end_pos = IfPush_next_elmpos - 1;

		if (IfPush_elm_end_pos < (qfh.qh.dval.elm_next_shift_pos - 1))
		{
		/* 先頭に領域がある --> 循環 */

/*
本来はこの条件判定は IfPush_elm_end_pos < qfh.qh.dval.elm_next_shift_pos で良いのだが
そーすると "次回 push 位置" == "次回 shift 位置" になってしまい、その後が
面倒なので IfPush_elm_end_pos と qfh.qh.dval.elm_next_shift_pos の間に 1 byte 空ける
*/

			/* push 位置へ移動 */
			elm_pos = qfh.qh.sval.elmseg_start_pos;

			/* 要素ヘッダを書き換え */
			ioeb.eh.prev_elmpos = qfh.qh.dval.elm_next_pop_pos;

			/* 属性ヘッダを書き換え */
			qfh.qh.dval.elm_next_pop_pos = qfh.qh.sval.elmseg_start_pos;
			qfh.qh.dval.elm_next_push_pos = IfPush_next_elmpos;

			if (qfh.qh.dval.elm_num == 0)
			{
			/* 次回の shift 位置も書き換え */

				qfh.qh.dval.elm_next_shift_pos = qfh.qh.sval.elmseg_start_pos;
			}
		}
		else
		{
			SFQ_FAIL_SILENT(W_NOSPACE);
		}
	}

//#ifdef SFQ_DEBUG_BUILD
//	sfq_print_e_header(&ioeb.eh);
//#endif

/* add element */
	b = sfq_writeelm(qo, elm_pos, &ioeb);
	if (! b)
	{
		SFQ_FAIL(EA_ELMRW, "sfq_writeelm");
	}

	if (qfh.qh.dval.elm_num == 0)
	{
	/* 初回 */

		/* 次回の shift 位置を先頭位置に移動 */
		qfh.qh.dval.elm_next_shift_pos = qfh.qh.sval.elmseg_start_pos;
	}

	if (ioeb.eh.prev_elmpos)
	{
/* update prev-element */
		/* next_elmpos を書き換え、リンクをつなげる */

		b = sfq_link_nextelm(qo, ioeb.eh.prev_elmpos, elm_pos);
		if (! b)
		{
			SFQ_FAIL(EA_SEEKSETIO, "sfq_link_nextelm(prev_eh)");
		}
	}

/* update procs */
	if (questate & SFQ_QST_EXEC_ON)
	{
		if (procs)
		{
			struct stat stbuf;
			sfq_bool try_resreve = SFQ_false;

/*
プロセステーブルが存在するので、queue ファイルのパーミッションを確認して
当該プロセスに実行権限があれば実行予約を試みる
*/
			if (fstat(fileno(qo->fp), &stbuf) != 0)
			{
				SFQ_FAIL(ES_FILE, "fstat(qo->fp)");
			}

			if (stbuf.st_mode & S_IXOTH)
			{
			/* o+x */
				try_resreve = SFQ_true;
			}
			else if (stbuf.st_mode & S_IXUSR)
			{
			/* u+x */
				if (stbuf.st_uid == geteuid())
				{
					try_resreve = SFQ_true;
				}
			}
			else if (stbuf.st_mode & S_IXGRP)
			{
			/* g+x */
				if (stbuf.st_gid == getegid())
				{
					try_resreve = SFQ_true;
				}
			}

			if (try_resreve)
			{
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

/*
正常時には呼び出し元に uuid を返却する
*/
	uuid_copy(val->uuid, ioeb.eh.uuid);

/* update queue file header */

	/* 要素数を加算 */
	qfh.qh.dval.elm_num++;

	/* 要素サイズを加算 */
	qfh.qh.dval.elmsize_total_ += ioeb.eh.elmsize_;

/* update header */
	b = sfq_writeqfh(qo, &qfh, procs, "PSH");
	if (! b)
	{
		SFQ_FAIL(EA_QFHRW, "sfq_writeqfh");
	}

	if (slotno != -1)
	{
		if (slotno_ptr)
		{
			(*slotno_ptr) = (short)slotno;
		}

		if (questate_ptr)
		{
			(*questate_ptr) = questate;
		}
	}

SFQ_LIB_CHECKPOINT

	free(wrkdir);
	wrkdir = NULL;

	free(full_soutpath);
	full_soutpath = NULL;

	free(full_serrpath);
	full_serrpath = NULL;

	free(procs);
	procs = NULL;

SFQ_LIB_LEAVE

	return SFQ_LIB_RC();
}

int sfq_push(const char* querootdir, const char* quename, struct sfq_value* val)
{
	struct sfq_queue_object* qo = NULL;

	short slotno = -1;
	questate_t questate = 0;

	int irc = -1;

SFQ_ENTP_ENTER

/* open queue-file */
	qo = sfq_open_queue_rw(querootdir, quename);
	if (! qo)
	{
		SFQ_FAIL(EA_OPENQUEUE, "sfq_open_queue_rw");
	}

	irc = sfq_push_(qo, val, &slotno, &questate);
	if (irc != SFQ_RC_SUCCESS)
	{
		SFQ_FAIL_V(irc, "sfq_push_() return %d", irc);
	}

	sfq_close_queue(qo);
	qo = NULL;

	if (slotno != -1)
	{
		sfq_go_exec(querootdir, quename, slotno, questate);
	}

SFQ_LIB_CHECKPOINT

	sfq_close_queue(qo);
	qo = NULL;

SFQ_ENTP_LEAVE

	return SFQ_LIB_RC();
}

int sfq_push_text(const char* querootdir, const char* quename,
	const char* eworkdir, const char* execpath, const char* execargs,
	const char* metatext, const char* soutpath, const char* serrpath,
	uuid_t uuid,
	const char* textdata)
{
	int irc = 0;
	struct sfq_value val;

	bzero(&val, sizeof(val));

	val.eworkdir = eworkdir;
	val.execpath = execpath;
	val.execargs = execargs;
	val.metatext = metatext;
	val.soutpath = soutpath;
	val.serrpath = serrpath;

	if (textdata)
	{
		val.payload_type = SFQ_PLT_CHARARRAY | SFQ_PLT_NULLTERM;
		val.payload = (sfq_byte*)textdata;
	}

	irc = sfq_push(querootdir, quename, &val);

	if (uuid)
	{
		if (irc == SFQ_RC_SUCCESS)
		{
			uuid_copy(uuid, val.uuid);
		}
	}

	return irc;
}

int sfq_push_binary(const char* querootdir, const char* quename,
	const char* eworkdir, const char* execpath, const char* execargs,
	const char* metatext, const char* soutpath, const char* serrpath,
	uuid_t uuid,
	const sfq_byte* payload, size_t payload_size)
{
	int irc = 0;
	struct sfq_value val;

	bzero(&val, sizeof(val));

	val.eworkdir = eworkdir;
	val.execpath = execpath;
	val.execargs = execargs;
	val.metatext = metatext;
	val.soutpath = soutpath;
	val.serrpath = serrpath;

	if (payload)
	{
		val.payload_type = SFQ_PLT_BINARY;
		val.payload_size = payload_size;
		val.payload = payload;
	}

	irc = sfq_push(querootdir, quename, &val);

	if (uuid)
	{
		if (irc == SFQ_RC_SUCCESS)
		{
			uuid_copy(uuid, val.uuid);
		}
	}

	return irc;
}

