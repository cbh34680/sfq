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

void sfq_write_execrc(const char* logdir, const uuid_t uuid, int rc)
{
	const char* rcfn = "rc.txt";
	char* rcfpath = NULL;
	size_t rcfpath_size = 0;

	char subdir[47 + 1];

	uuid2subdir(uuid, subdir, sizeof(subdir));

	/* "logdir/subdir\0" */
	rcfpath_size =
	(
		strlen(logdir)		+ /* logdir */
		1			+ /* "/"    */
		strlen(subdir)		+ /* subdir */
		1			+ /* "/"    */
		strlen(rcfn)		+ /* rcfn   */
		1			  /* "\0"   */
	);

	rcfpath = alloca(rcfpath_size);
	if (rcfpath)
	{
		FILE* rcfp = NULL;

		snprintf(rcfpath, rcfpath_size, "%s/%s/%s", logdir, subdir, rcfn);

		rcfp = fopen(rcfpath, "wt");
		if (rcfp)
		{
			fprintf(rcfp, "%d\n", rc);

			fclose(rcfp);
			rcfp = NULL;
		}
	}
}

static const char* mkdir_and_alloc_wpath_4exec(const char* logdir, const uuid_t uuid,
	ulong id, const char* ext, mode_t dir_perm)
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
			if (sfq_mkdir_p(outdir, dir_perm))
			{
				/* go next */

				setenv("SFQ_EXECLOGDIR", outdir, 0);
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
		const char* basename = "std";

		/* "dir/0.{out,err}\0" */
		size_t wpath_size =
		(
			strlen(outdir)		+ /* outdir */
			1			+ /* "/"    */
			strlen(basename)	+ /* "std"  */
			1			+ /* "."    */
			strlen(ext)		+ /* ext    */
			1			  /* "\0"   */
		);

		wpath = malloc(wpath_size);
		if (wpath)
		{
			const char* idfn = "id.txt";
			char* idfpath = NULL;
			size_t idfpath_size = 0;

/* set return value */
			snprintf(wpath, wpath_size, "%s/%s.%s", outdir, basename, ext);

/* write [id] to file("id.txt") */
			idfpath_size = (
				strlen(outdir)		+ /* outdir   */
				1			+ /* "/"      */
				strlen(idfn)		+ /* "id.txt" */
				1			  /* "\0"     */
			);

			idfpath = alloca(idfpath_size);
			if (idfpath)
			{
				FILE* idfp = NULL;

				snprintf(idfpath, idfpath_size, "%s/%s", outdir, idfn);

				if (stat(idfpath, &stbuf) == 0)
				{
					/* go next */
				}
				else
				{
					idfp = fopen(idfpath, "w");
					if (idfp)
					{
						fprintf(idfp, "%zu\n", id);

						fclose(idfp);
						idfp = NULL;

						setenv("SFQ_IDFILEPATH", idfpath, 0);
					}
				}
			}
		}
	}

	return wpath;
}

void sfq_output_reopen_4exec(FILE* fp, const time_t* now, const char* arg_wpath,
	const char* logdir, const uuid_t uuid, ulong id, const char* ext, const char* env_key,
	mode_t dir_perm, mode_t file_perm)
{
	int irc = -1;
	const char* opened = NULL;

fprintf(stderr, "\tattempt to re-open(%s) file\n", ext);

	if (arg_wpath)
	{
 		assert(arg_wpath[0]);

/* len(arg_wpath) > 0 */

		if (strcmp(arg_wpath, "-") == 0)
		{
			/* "-o -" "-e -" はデフォルトのログファイルとする */

			if (logdir)
			{
				const char* freopen_path = NULL;

fprintf(stderr, "\tmkdir(%s) [perm=%x] for default log\n", ext, dir_perm);
				freopen_path = mkdir_and_alloc_wpath_4exec(logdir, uuid, id, ext, dir_perm);

				if (freopen_path)
				{
fprintf(stderr, "\tfreopen(%s) file [mode=wb path=%s]\n", ext, freopen_path);

/*
freopen_path は uuid をパスに含んでいるので重複しない
--> "wb" で開く
*/
					if (freopen(freopen_path, "wb", fp))
					{
						int fd = -1;

						fd = fileno(fp);
						if (fd != -1)
						{
fprintf(stderr, "\tchmod(%s) [perm=%x path=%s]\n", ext, file_perm, freopen_path);
							irc = fchmod(fd, file_perm);

							if (irc != 0)
							{
perror("\t\tfchmod");
								/* ignore errors */
							}
						}

						opened = sfq_stradup(freopen_path);
					}
					else
					{
perror("\t\tfreopen error");
					}
				}

				free((char*)freopen_path);
				freopen_path = NULL;
			}
		}
		else
		{
			char freopen_mode[] = "wb";

			const char* parse_path = NULL;
			const char* freopen_path = NULL;

			if (strlen(arg_wpath) >= 3)
			{
/*
ファイルを開くモード (a, w) を指定できるように、"-e", "-o" は指定されたパスを解析する。

"-e a,path/to/file.txt" のように最初の 2 文字が "a,", "w," であった場合は freopen() へのパラメータとする
*/
				if (arg_wpath[1] == ',')
				{
					switch (arg_wpath[0])
					{
						case 'a':
						case 'w':
						{
							/* "a,X", "w,X" "a,XYZ" "w,xyz" ... */

							freopen_mode[0] = arg_wpath[0];
							parse_path = &arg_wpath[2];

							break;
						}
					}
				}
			}

			if (! parse_path)
			{
				parse_path = arg_wpath;
			}

			if (strchr(parse_path, '%'))
			{
				struct tm tmbuf;

/*
"-e mylog/%Y/%m/%d/%H%M%S.log" のように指定されていたとき (パスに "%" を含むとき)
は strftime() による変換を試みる
*/
				if (localtime_r(now, &tmbuf))
				{
					char tmp_s[BUFSIZ];
					size_t wsize = 0;

					wsize = strftime(tmp_s, sizeof(tmp_s), parse_path, &tmbuf);

					if (wsize != 0)
					{
						freopen_path = tmp_s;
					}
				}
			}
			else
			{
				freopen_path = parse_path;
			}

			if (freopen_path)
			{
				char* freopen_path_copy = sfq_stradup(freopen_path);
				if (freopen_path_copy)
				{
					char* dir = dirname(freopen_path_copy);
					if (dir)
					{
						char* cwd = getcwd(NULL, 0);

fprintf(stderr, "\tsfq_mkdir_p(%s) dir [cwd=%s path=%s perm=%x]\n", ext, cwd, dir, dir_perm);

						if (sfq_mkdir_p(dir, dir_perm))
						{
							sfq_bool firstOpen = SFQ_false;
							struct stat stbuf;

							firstOpen = (stat(freopen_path, &stbuf) == 0) ? SFQ_false : SFQ_true;

fprintf(stderr, "\tfreopen(%s) file [cwd=%s path=%s mode=%s first=%d]\n", ext, cwd, freopen_path, freopen_mode, firstOpen);

							if (freopen(freopen_path, freopen_mode, fp))
							{
								if (firstOpen)
								{
									int fd = -1;

									fd = fileno(fp);
									if (fd != -1)
									{
fprintf(stderr, "\tchmod(%s) [perm=%x path=%s]\n", ext, file_perm, freopen_path);
										irc = fchmod(fd, file_perm);

										if (irc != 0)
										{
perror("\t\tfchmod");
											/* ignore errors */
										}
									}
								}

								opened = freopen_path;
							}
							else
							{
perror("\t\tfreopen error");
							}
						}
						else
						{
perror("\t\tmkdir");
						}

						free(cwd);
						cwd = NULL;
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
fprintf(stderr, "\tre-open(%s) [mode=wb path=/dev/null]\n", ext);

		freopen("/dev/null", "wb", fp);
	}
}

static sfq_bool output_reopen_4proc(FILE* fp, const char* logdir, ushort slotno, const char* ext, mode_t file_perm)
{
	char* wpath = NULL;
	size_t wpath_size = 0;
	struct stat stbuf;

	/* "dir/slotno.{out,err}\0" */
	wpath_size =
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
		sfq_bool firstOpen = SFQ_false;

		snprintf(wpath, wpath_size, "%s/%u.%s", logdir, slotno, ext);

		firstOpen = (stat(wpath, &stbuf) == 0) ? SFQ_false : SFQ_true;

		if (freopen(wpath, "at", fp))
		{
			if (firstOpen)
			{
/*
最初に開いたときはパーミッションを変更する
*/
				int fd = -1;

				fd = fileno(fp);
				if (fd != -1)
				{
					fchmod(fd, file_perm);
				}
			}

			return SFQ_true;
		}
	}

	return SFQ_false;
}

void sfq_reopen_4proc(const char* logdir, ushort slotno,
	questate_t questate, mode_t file_perm)
{
	sfq_bool sout_ok = SFQ_false;
	sfq_bool serr_ok = SFQ_false;

	/* stdio */
	freopen("/dev/null", "rb", stdin);

	if (logdir)
	{
		/* stdout */
		if (questate & SFQ_QST_STDOUT_ON)
		{
			if (output_reopen_4proc(stdout, logdir, slotno, "out", file_perm))
			{
				sout_ok = SFQ_true;
			}
		}

		/* stderr */
		if (questate & SFQ_QST_STDERR_ON)
		{
			if (output_reopen_4proc(stderr, logdir, slotno, "err", file_perm))
			{
				serr_ok = SFQ_true;
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

