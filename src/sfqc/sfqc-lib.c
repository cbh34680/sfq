#include "sfqc-lib.h"

char** sfqc_split(char* params_str, char c_delim)
{
	int i = 0;

	char** params = NULL;
	int alloc_num = 0;

	char* pos = NULL;
	char* token = NULL;
	char* saveptr = NULL;

	char delim[2] = { c_delim, '\0' };

	if (! params_str)
	{
		return NULL;
	}

/* count ',' */
	pos = params_str;
	while (*pos)
	{
		if ((*pos) == c_delim)
		{
			alloc_num++;
		}
		pos++;
	}
	alloc_num++;

/* alloc char*[] */
	params = (char**)calloc(sizeof(char*), alloc_num + 1);
	if (! params)
	{
		return NULL;
	}

/* set to char*[] */
	for (i=0, pos=params_str; i<alloc_num; i++, pos=NULL)
	{
		token = strtok_r(pos, delim, &saveptr);
		if (token == NULL)
		{
			break;
		}

		params[i] = token;
	}

	return params;
}

int sfqc_readstdin(sfq_byte** mem_ptr, size_t* memsize_ptr)
{
	int ret = 1;

	sfq_byte* mem = NULL;
	size_t memsize = 0;
	sfq_byte buf[BUFSIZ];
	size_t n = 1;

/* */
	while (! feof(stdin))
	{
		n = fread(buf, 1, sizeof(buf), stdin);

		if (ferror(stdin))
		{
			goto EXIT_LABEL;
		}

		if (n)
		{
/* allocate mem */
			mem = realloc(mem, memsize + n + 1);
			if (! mem)
			{
				goto EXIT_LABEL;
			}

			memcpy(&mem[memsize], buf, n);
			memsize += n;
		}
	}

	if (mem)
	{
		/* null term for push-text */
		mem[memsize] = '\0';
	}

	(*mem_ptr) = mem;

	if (memsize_ptr)
	{
		(*memsize_ptr) = memsize;
	}

	ret = 0;

EXIT_LABEL:

	if (ret != 0)
	{
		free(mem);
		mem = NULL;
	}

	return ret;
}

int sfqc_readfile(const char* path, sfq_byte** mem_ptr, size_t* memsize_ptr)
{
	int ret = 1;

	sfq_byte* mem = NULL;
	FILE* fp = NULL;
	struct stat st;
	size_t n = 0;
	int irc = 0;

	irc = stat(path, &st);
	if (irc != 0)
	{
		goto EXIT_LABEL;
	}

	if (st.st_size > 0)
	{
		fp = fopen(path, "rb");
		if (! fp)
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

		/* null term for push-text */
		mem[st.st_size] = '\0';

	}

	(*mem_ptr) = mem;

	if (memsize_ptr)
	{
		(*memsize_ptr) = st.st_size;
	}

	ret = 0;

EXIT_LABEL:

	if (ret != 0)
	{
		free(mem);
		mem = NULL;
	}

	if (fp)
	{
		fclose(fp);
		fp = NULL;
	}

	return ret;
}

/* プログラム引数の解析 */
void sfqc_free_init_option(struct sfqc_init_option* p)
{
	if (! p)
	{
		return;
	}

	free((char*)p->querootdir);
	free((char*)p->quename);
	free((char*)p->queuser);
	free((char*)p->quegroup);
	free((char*)p->execpath);
	free((char*)p->execargs);
	free((char*)p->metatext);
	free((char*)p->textdata);
	free((char*)p->inputfile);
	free((char*)p->soutpath);
	free((char*)p->serrpath);
	free((char*)p->printmethod);

	bzero(p, sizeof(struct sfqc_init_option));
}

int get_ul_bytesize(const char* str, unsigned long* ul_ptr, char** e_ptr)
{
	ulong mul = 0;

	char* e = NULL;
	unsigned long ul = strtoul(str, &e, 0);

	if (*e)
	{
		(*e_ptr) = e;

		if ((strlen(e) == 1) || ((strlen(e) == 2) && (((e[1] == 'b') || (e[1] == 'B')))))
		{
			switch (*e)
			{
				case 'k':
				case 'K':
				{
					mul = (1024);
					break;
				}

				case 'm':
				case 'M':
				{
					mul = (1024 * 1024);
					break;
				}

				case 'g':
				case 'G':
				{
					mul = (1024 * 1024 * 1024);
					break;
				}
			}
		}

		if (mul == 0)
		{
			return 1;
		}
		else
		{
			if (ul == 0)
			{
				ul = 1;
			}
		}
	}

	if (mul)
	{
		ul *= mul;
	}

	(*ul_ptr) = ul;

	return 0;
}

#define RESET_STR(org, p, MEMBER) \
	char* c = strdup(org); \
	if (! c) { \
		snprintf(message, sizeof(message), "'%c': mem alloc", opt); \
		jumppos = __LINE__; \
		goto EXIT_LABEL; \
	} \
	free((char*)p->MEMBER); \
	p->MEMBER = c;

int sfqc_get_init_option(int argc, char** argv, const char* optstring, bool use_rest, struct sfqc_init_option* p)
{
	int irc = 1;
	int opt = 0;

	int jumppos = -1;
	char message[128] = "";

	assert(p);

/* */
	bzero(p, sizeof(*p));

	while ((opt = getopt(argc, argv, optstring)) != -1)
	{
		switch (opt)
		{
			case 'S':
			{
				/* ファイルサイズの最大 */
				unsigned long ul = 0;
				char* e = NULL;

				if (get_ul_bytesize(optarg, &ul, &e) != 0)
				{
					snprintf(message, sizeof(message), "'%c': ignore [%s]", opt, e);
					jumppos = __LINE__;
					goto EXIT_LABEL;
				}

				p->filesize_limit = ul;

				break;
			}
			case 'L':
			{
				/* 要素サイズの最大 */
				unsigned long ul = 0;
				char* e = NULL;

				if (get_ul_bytesize(optarg, &ul, &e) != 0)
				{
					snprintf(message, sizeof(message), "'%c': ignore [%s]", opt, e);
					jumppos = __LINE__;
					goto EXIT_LABEL;
				}

				p->payloadsize_limit = ul;

				break;
			}
			case 'B':
			{
				/* 最大プロセス数 */
				char* e = NULL;
				unsigned long ul = 0;

				ul =strtoul(optarg, &e, 0);
				if (*e)
				{
					snprintf(message, sizeof(message), "'%c': ignore [%s]", opt, e);
					jumppos = __LINE__;
					goto EXIT_LABEL;
				}
				if (ul >= USHRT_MAX)
				{
					snprintf(message, sizeof(message), "'%c': size over (%u)", opt, USHRT_MAX);
				}

				p->boota_proc_num = (ushort)ul;

				break;
			}

			case 'N': { RESET_STR(optarg, p, quename);	break; } // QUEUE 名
			case 'D': { RESET_STR(optarg, p, querootdir);	break; } // QUEUE ディレクトリ
			case 'U': { RESET_STR(optarg, p, queuser);	break; } // QUEUE ユーザ
			case 'G': { RESET_STR(optarg, p, quegroup);	break; } // QUEUE グループ

			case 'x': { RESET_STR(optarg, p, execpath);	break; } // exec() パス
			case 'a': { RESET_STR(optarg, p, execargs);	break; } // exec() 引数 (カンマ区切り)
			case 't': { RESET_STR(optarg, p, textdata);	break; } // データ# テキスト
			case 'f': { RESET_STR(optarg, p, inputfile);	break; } // データ# ファイル名
			case 'm': { RESET_STR(optarg, p, metatext);	break; } // メタ情報
			case 'p': { RESET_STR(optarg, p, printmethod);	break; } // pop, shift の出力方法

			// 標準出力のリダイレクト先
			case 'o': { RESET_STR(optarg ? optarg : "-", p, soutpath); break; }

			// 標準エラーのリダイレクト先
			case 'e': { RESET_STR(optarg ? optarg : "-", p, serrpath); break; }

			case 'q':
			{
				p->quiet = true;
				break;
			}

/* */
			case '?':
			default:
			{
				snprintf(message, sizeof(message), "'%c': unknown route", opt);
				jumppos = __LINE__;
				goto EXIT_LABEL;

				break;
			}
		}
	}

	irc = 0;

EXIT_LABEL:

	if (irc != 0)
	{
		if (message[0])
		{
			fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
		}

		sfqc_free_init_option(p);

		return irc;
	}

	if (p->filesize_limit == 0)
	{
		/* default 256MB */
		p->filesize_limit = (256 * 1024 * 1024);
	}

	if (use_rest)
	{
		p->commands = (const char**)&argv[optind];
		p->command_num = argc - optind;
	}
	else
	{
		int i = 0;

		for (i=optind; i<argc; i++)
		{
			fprintf(stderr, "%s(%d): [%s] ignore argument\n" , __FILE__, __LINE__, argv[i]);
		}

		if (argc != optind)
		{
			fprintf(stderr, "\n");
		}
	}

	return irc;
}

