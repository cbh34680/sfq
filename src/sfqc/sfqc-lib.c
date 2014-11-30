#include "sfqc-lib.h"

#if 0
static bool is_dir(const char* path)
{
	struct stat buf;

	if (stat(path, &buf) == 0)
	{
		if (S_ISDIR(buf.st_mode))
		{
			return true;
		}
	}

	return false;
}

static bool is_file(const char* path)
{
	struct stat buf;

	if (stat(path, &buf) == 0)
	{
		if (S_ISREG(buf.st_mode))
		{
			return true;
		}
	}

	return false;
}
#endif

sfq_byte* sfqc_readstdin(size_t* readsize)
{
	sfq_byte* mem = NULL;
	size_t memsize = 0;
	sfq_byte buf[BUFSIZ];
	size_t n = 1;

	while (n)
	{
		n = fread(buf, 1, sizeof(buf), stdin);
		if (n)
		{
/* allocate mem */
			mem = realloc(mem, memsize + n + 1);
			if (! mem)
			{
				return NULL;
			}

			memcpy(&mem[memsize], buf, n);
			memsize += n;
		}
	}

	if (memsize == 0)
	{
		return NULL;
	}

	mem[memsize] = '\0';

	if (readsize)
	{
		(*readsize) = memsize;
	}

	return mem;
}

sfq_byte* sfqc_readfile(const char* path, size_t* readsize)
{
	sfq_byte* mem = NULL;
	FILE* fp = NULL;
	struct stat st;
	int rc = -1;
	size_t n = 0;

	rc = stat(path, &st);
	if (rc != 0)
	{
		goto EXIT_LABEL;
	}

	fp = fopen(path, "r");
	if (! fp)
	{
		goto EXIT_LABEL;
	}

	if (st.st_size == 0)
	{
		goto EXIT_LABEL;
	}

	mem = malloc(st.st_size + 1);
	if (! mem)
	{
		goto EXIT_LABEL;
	}

	n = fread(mem, st.st_size, 1, fp);
	if (n != 1)
	{
		goto EXIT_LABEL;
	}

	mem[st.st_size] = '\0';

	if (readsize)
	{
		(*readsize) = st.st_size;
	}

	rc = 0;

EXIT_LABEL:

	if (fp)
	{
		fclose(fp);
		fp = NULL;
	}

	if (rc != 0)
	{
		free(mem);
		mem = NULL;
	}

	return mem;
}

/* プログラム引数の解析 */
void sfqc_free_init_option(struct sfqc_init_option* p)
{
	if (! p)
	{
		return;
	}

	free(p->querootdir);
	free(p->quename);
	free(p->execpath);
	free(p->execargs);
	free(p->metadata);
	free(p->textdata);
	free(p->inputfile);

	bzero(p, sizeof(struct sfqc_init_option));
}

int sfqc_can_push(const struct sfqc_init_option* p)
{
	if (p)
	{
		if (p->execpath || p->execargs || p->textdata || p->inputfile)
		{
			if (p->textdata && p->inputfile)
			{
				/* go next ... false ('-f' && '-t') */
			}
			else
			{
				return SFQ_RC_SUCCESS;
			}
		}
	}

	return SFQ_RC_EA_FUNCARG;
}

int sfqc_get_init_option(int argc, char** argv, const char* optstring, struct sfqc_init_option* p)
{
	int opt = 0;
	int irc = SFQ_RC_UNKNOWN;
	char errstr[32] = "";

	assert(p);

/* */
	bzero(p, sizeof(*p));

	while ((opt = getopt(argc, argv, optstring)) != -1)
	{
		switch (opt)
		{
			case '?':
			{
				snprintf(errstr, sizeof(errstr), "%c:unknown option", optopt);
				goto EXIT_LABEL;

				break;
			}
			case 'N':
			{
				/* QUEUE 名 */
				char* c = strdup(optarg);
				if (! c)
				{
					snprintf(errstr, sizeof(errstr), "%c:mem alloc", opt);
					goto EXIT_LABEL;
				}
				p->quename = c;

				break;
			}
			case 'D':
			{
				/* QUEUE ディレクトリ */
				char* c = strdup(optarg);
				if (! c)
				{
					snprintf(errstr, sizeof(errstr), "%c:mem alloc", opt);
					goto EXIT_LABEL;
				}
				p->querootdir = c;

				break;
			}
			case 'S':
			{
				/* ファイルサイズの最大 */
				char* e = NULL;
				unsigned long ul = strtoul(optarg, &e, 0);
				if (*e)
				{
					snprintf(errstr, sizeof(errstr), "%c:ignore [%s]", opt, e);
					goto EXIT_LABEL;
				}
				p->filesize_limit = ul;

				break;
			}
			case 'L':
			{
				/* 要素サイズの最大 */
				char* e = NULL;
				unsigned long ul = strtoul(optarg, &e, 0);
				if (*e)
				{
					snprintf(errstr, sizeof(errstr), "%c:ignore [%s]", opt, e);
					goto EXIT_LABEL;
				}
				p->payloadsize_limit = ul;

				break;
			}
			case 'P':
			{
				/* 最大プロセス数 */
				char* e = NULL;
				unsigned long ul = strtoul(optarg, &e, 0);
				if (*e)
				{
					snprintf(errstr, sizeof(errstr), "%c:ignore [%s]", opt, e);
					goto EXIT_LABEL;
				}
				if (ul >= USHRT_MAX)
				{
					snprintf(errstr, sizeof(errstr), "%c:size over (%u)", opt, USHRT_MAX);
				}
				p->max_proc_num = (ushort)ul;

				break;
			}
			case 'x':
			{
				/* exec() path */
				char* c = strdup(optarg);
				if (! c)
				{
					snprintf(errstr, sizeof(errstr), "%c:mem alloc", opt);
					goto EXIT_LABEL;
				}
				p->execpath = c;

				break;
			}
			case 'a':
			{
				/* exec() args */
				char* c = strdup(optarg);
				if (! c)
				{
					snprintf(errstr, sizeof(errstr), "%c:mem alloc", opt);
					goto EXIT_LABEL;
				}
				p->execargs = c;

				break;
			}
			case 't':
			{
				/* テキスト */
				char* c = strdup(optarg);
				if (! c)
				{
					snprintf(errstr, sizeof(errstr), "%c:mem alloc", opt);
					goto EXIT_LABEL;
				}
				p->textdata = c;

				break;
			}
			case 'f':
			{
				/* ファイル名 */
				char* c = strdup(optarg);
				if (! c)
				{
					snprintf(errstr, sizeof(errstr), "%c:mem alloc", opt);
					goto EXIT_LABEL;
				}
				p->inputfile = c;

				break;
			}
			case 'm':
			{
				/* メタ情報 */
				char* c = strdup(optarg);
				if (! c)
				{
					snprintf(errstr, sizeof(errstr), "%c:mem alloc", opt);
					goto EXIT_LABEL;
				}
				p->metadata = c;

				break;
			}
			default:
			{
				snprintf(errstr, sizeof(errstr), "%c:unknown route", opt);
				goto EXIT_LABEL;

				break;
			}
		}
	}

	irc = SFQ_RC_SUCCESS;

EXIT_LABEL:

	if (irc != SFQ_RC_SUCCESS)
	{
		if (errstr[0])
		{
			fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, errstr);
		}

		sfqc_free_init_option(p);

		return irc;
	}

	if (p->filesize_limit == 0)
	{
		p->filesize_limit = (256 * 1024 * 1024);
	}

/*
	p->filesize_limit *= (1024 * 1024);

	if (p->payloadsize_limit)
	{
		p->payloadsize_limit *= (1024 * 1024);
	}
*/

/*
	while (optind < argc)
	{
		fprintf(stdout, "[%s] を処理します\n" , argv[optind++]);
	}
*/

	return irc;
}

