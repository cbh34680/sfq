#include "sfq-lib.h"

static void uuid2subdir(const uuid_t uuid, char* subdir, size_t subdir_size)
{
	char uuid_s[36 + 1];

	/*  123456789012345678901234567890123456      12345678901234567890123456789012345678901234567  */
	/*  _ _ _ _  _ _  _ _  _ _  _ _ _ _ _ _ */
	/*  012345678901234567890123456789012345      1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16  */
	/*  1 2 3 4  5 6  7 8  9 0  1 2 3 4 5 6 */
	/* "1b4e28ba-2fa1-11d2-883f-0016d3cca427" -> "1b/4e/28/ba/2f/a1/11/d2/88/3f/00/16/d3/cc/a4/27" */

	uuid_unparse(uuid, uuid_s);

	snprintf(subdir, subdir_size, "%.2s/%.2s/%.2s/%.2s/%.2s/%.2s/%.2s/%.2s/%.2s/%.2s/%.2s/%.2s/%.2s/%.2s/%.2s/%.2s",
		&uuid_s[0],  &uuid_s[2],  &uuid_s[4],  &uuid_s[6],
		&uuid_s[9],  &uuid_s[11],
		&uuid_s[14], &uuid_s[16],
		&uuid_s[19], &uuid_s[21],
		&uuid_s[24], &uuid_s[26], &uuid_s[28], &uuid_s[30], &uuid_s[32], &uuid_s[34]
	);
}

static char* alloc_wpath_4exec(const char* logdir, const uuid_t uuid, ulong id, const char* ext)
{
	char subdir[47 + 1];

	char* outdir = NULL;
	char* wpath = NULL;

	size_t outdir_size = 0;

	struct stat stbuf;

/* */
	uuid2subdir(uuid, subdir, sizeof(subdir));

	/* "logdir/subdir\0" */
	outdir_size = (strlen(logdir) + 1 + strlen(subdir) + 1 /* "\0" */);

	outdir = alloca(outdir_size);
	if (outdir)
	{
		snprintf(outdir, outdir_size, "%s/%s", logdir, subdir);
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

		wpath = malloc(wpath_size);
		if (wpath)
		{
			snprintf(wpath, wpath_size, "%s/%zu.%s", outdir, id, ext);
		}
	}

	return wpath;
}

void sfq_output_reopen_4exec(FILE* fp, const time_t* now, const char* arg_wpath,
	const char* logdir, const uuid_t uuid, ulong id, const char* ext, const char* env_key)
{
	const char* opened = NULL;

	if (arg_wpath && (arg_wpath[0] != '\0'))
	{
/* len(arg_wpath) > 0 */

		if (strcmp(arg_wpath, "-") == 0)
		{
			/* "-o -" "-e -" はデフォルトのログファイルとする */

			if (logdir)
			{
				char* wpath = NULL;

				wpath = alloc_wpath_4exec(logdir, uuid, id, ext);
				if (wpath)
				{
fprintf(stderr, "\topen(%s) file [mode=wb path=%s] for default log\n", ext, wpath);

					if (freopen(wpath, "wb", fp))
					{
						opened = sfq_stradup(wpath);
					}
					else
					{
perror("\t\terror");
					}
				}

				free(wpath);
				wpath = NULL;
			}
		}
		else
		{
			char wmode[] = "wb";
			const char* path1 = NULL;
			const char* path2 = NULL;

			if (strlen(arg_wpath) >= 3)
			{
				if (arg_wpath[1] == ',')
				{
					switch (arg_wpath[0])
					{
						case 'a':
						case 'w':
						{
							/* "a,X", "w,X" "a,XYZ" "w,xyz" ... */

							wmode[0] = arg_wpath[0];
							path1 = &arg_wpath[2];

							break;
						}
					}
				}

				if (! path1)
				{
					path1 = arg_wpath;
				}
			}
			else
			{
				path1 = arg_wpath;
			}

			if (strchr(path1, '%'))
			{
				struct tm tmp_tm;

				if (localtime_r(now, &tmp_tm))
				{
					char tmp_s[BUFSIZ];
					size_t wsize = 0;

					wsize = strftime(tmp_s, sizeof(tmp_s), path1, &tmp_tm);

					if (wsize != 0)
					{
						path2 = tmp_s;
					}
				}
			}
			else
			{
				path2 = path1;
			}

			if (path2)
			{
				char* dir = sfq_stradup(path2);
				if (dir)
				{
					if (dirname(dir))
					{
						if (sfq_mkdir_p(dir, 0700))
						{
fprintf(stderr, "\topen(%s) file [mode=%s path=%s] for specified log\n", ext, wmode, path2);

							if (freopen(path2, wmode, fp))
							{
								opened = path2;
							}
							else
							{
perror("\t\terror");
							}
						}
					}
				}
			}
		}
	}

	if (opened)
	{
/*
通常のファイルを書込みモードで開けたら環境変数に残す
*/
		char* rpath = realpath(opened, NULL);
		if (rpath)
		{
			setenv(env_key, rpath, 0);
		}

		free(rpath);
		rpath = NULL;
	}
	else
	{
fprintf(stderr, "\topen(%s) [mode=wb path=/dev/null]\n", ext);

		freopen("/dev/null", "wb", fp);
	}
}

static bool output_reopen_4proc(FILE* fp, const char* logdir, ushort slotno, const char* ext)
{
	char* wpath = NULL;
	size_t wpath_size = 0;

	/* "dir/slotno.{out,err}\0" */
	wpath_size = (
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

		if (freopen(wpath, "at", fp))
		{
			return true;
		}
	}

	return false;
}

void sfq_reopen_4proc(const char* logdir, ushort slotno, questate_t questate)
{
	bool sout_ok = false;
	bool serr_ok = false;

	/* stdio */
	freopen("/dev/null", "rb", stdin);

	if (logdir)
	{
		/* stdout */
		if (questate & SFQ_QST_STDOUT_ON)
		{
			if (output_reopen_4proc(stdout, logdir, slotno, "out"))
			{
				sout_ok = true;
			}
		}

		/* stderr */
		if (questate & SFQ_QST_STDERR_ON)
		{
			if (output_reopen_4proc(stderr, logdir, slotno, "err"))
			{
				serr_ok = true;
			}
		}
	}

	if (! sout_ok)
	{
		freopen("/dev/null", "at", stdout);
	}

	if (! serr_ok)
	{
		freopen("/dev/null", "at", stderr);
	}

/*
バッファリングを無効にしないと foreach_element() の printf() が
重複して出力されてしまう

原因は不明
*/
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}

