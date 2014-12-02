#include "sfq-lib.h"

static void output_reopen_4exec(FILE* fp, const char* logdir, time_t now, ulong id, const char* ext)
{
	char* wpath = NULL;

	if (logdir)
	{
		char* outdir = NULL;
		size_t outdir_size = 0;

		struct tm tmbuf;
		struct stat stbuf;

		localtime_r(&now, &tmbuf);

		outdir_size =
		(
			strlen(logdir)	+ /* logdir */
			1		+ /* "/"    */
			4		+ /* yyyy   */
			1		+ /* "/"    */
			4		+ /* mmdd   */
			1		+ /* "/"    */
			2		+ /* HH     */
			1		+ /* "/"    */
			2		+ /* MM     */
			1		+ /* "/"    */
			2		+ /* SS     */
			1		  /* "\0"   */
		);

		outdir = alloca(outdir_size);
		if (outdir)
		{
			snprintf(outdir, outdir_size, "%s/%04d/%02d%02d/%02d/%02d/%02d",
				logdir, tmbuf.tm_year + 1900, tmbuf.tm_mon + 1, tmbuf.tm_mday,
				tmbuf.tm_hour, tmbuf.tm_min, tmbuf.tm_sec);

			if (stat(outdir, &stbuf) == 0)
			{
				/* go next */
			}
			else
			{
				if (sfq_mkdir_p(outdir, 0700))
				{
					/* go next */
				}
				else
				{
					outdir = NULL;
					outdir_size = 0;
				}
			}
		}

		if (outdir)
		{
			/* "/proc/sys/kernel/pid_max" ... プロセス番号最大 */

			/* "dir/ppid-pid.{out,err}\0" */
			size_t wpath_size =
			(
				strlen(outdir)			+ /* outdir */
				1				+ /* "/"    */
				SFQ_PLUSINTSTR_WIDTH(id)	+ /* id     */
				1				+ /* "."    */
				strlen(ext)			+ /* ext    */
				1				  /* "\0"   */
			);

			wpath = alloca(wpath_size);
			if (wpath)
			{
				snprintf(wpath, wpath_size, "%s/%zu.%s", outdir, id, ext);
			}
		}
	}

	if (! wpath)
	{
		wpath = "/dev/null";
	}

	fflush(fp);

	if (! freopen(wpath, "ab", fp))
	{
		freopen("/dev/null", "ab", fp);
	}
}

static void output_reopen_4proc(FILE* fp, const char* logdir, ushort slotno, const char* ext)
{
	char* wpath = NULL;

	if (logdir)
	{
		/* "dir/slotno.{out,err}\0" */
		size_t wpath_size =
		(
			strlen(logdir)			+ /* logdir */
			1				+ /* "/"    */
			SFQ_PLUSINTSTR_WIDTH(slotno)	+ /* slotno */
			1				+ /* "."    */
			strlen(ext)			+ /* ext    */
			1				  /* "\0"   */
		);

		wpath = alloca(wpath_size);
		if (wpath)
		{
			snprintf(wpath, wpath_size, "%s/%u.%s", logdir, slotno, ext);
		}
	}

	if (! wpath)
	{
		wpath = "/dev/null";
	}

	fflush(fp);

	if (! freopen(wpath, "at", fp))
	{
		freopen("/dev/null", "at", fp);
	}
}

void sfq_reopen_4exec(const char* logdir, ulong id)
{
	time_t now = time(NULL);

	/* stdio */
	freopen("/dev/null", "rb", stdin);

	/* stdout */
	output_reopen_4exec(stdout, logdir, now, id, "out");

	/* stderr */
	output_reopen_4exec(stderr, logdir, now, id, "err");
}

void sfq_reopen_4proc(const char* logdir, ushort slotno)
{
	/* stdio */
	freopen("/dev/null", "rb", stdin);

	/* stdout */
	output_reopen_4proc(stdout, logdir, slotno, "out");

	/* stderr */
	output_reopen_4proc(stderr, logdir, slotno, "err");

/*
バッファリングを無効にしないと foreach_element() の printf() が
重複して出力されてしまう
*/
	setvbuf(stdout, NULL, _IONBF, 0);
}

