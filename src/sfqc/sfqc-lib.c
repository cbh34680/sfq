#include "sfqc-lib.h"

static void to_camelcase(char* p)
{
	char* pos = p;
	sfq_bool next_upper = SFQ_true;

	if (! p)
	{
		return;
	}

	while (*pos)
	{
		if (((*pos) == ' ') || ((*pos) == ':'))
		{
			break;
		}

		if ((*pos) == '-')
		{
			next_upper = SFQ_true;
		}
		else
		{
			if (next_upper)
			{
				(*pos) = toupper(*pos);
				next_upper = SFQ_false;
			}
		}

		pos++;
	}
}

void sfqc_h_printf(sfq_bool http, const char* org_format, ...)
{
	va_list arg;
	char* format = NULL;

	format = sfq_stradup(org_format);

	if (http)
	{
		printf("X-Sfq-");
		to_camelcase(format);
	}

#if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ > 2
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wvarargs"
#endif
	va_start(arg, format);
#if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ > 2
 #pragma GCC diagnostic pop
#endif
	vprintf(format, arg);
	va_end(arg);
}

static void print_date(const char* key, time_t t)
{
	struct tm tmp;
	char dt[64];

	gmtime_r(&t, &tmp);
	strftime(dt, sizeof dt, "%a, %d %b %Y %H:%M:%S %Z", &tmp);

	printf("%s: %s" SFQC_CRLF, key, dt);
}

void sfqc_print_http_headers(sfq_bool exist, const char* content_type, size_t content_length)
{
	time_t now = time(NULL);

	printf("HTTP/1.0 ");

	if (exist)
	{
		printf("200 OK" SFQC_CRLF);
	}
	else
	{
		printf("204 No Content" SFQC_CRLF);
	}

	printf("Content-Type: %s" SFQC_CRLF, content_type);
	printf("Content-Length: %zu" SFQC_CRLF, content_length);

	print_date("Date", now);
	print_date("Expires", now + 10);

	printf("Connection: close" SFQC_CRLF);
}

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
void sfqc_free_program_args(struct sfqc_program_args* pgargs)
{
	if (! pgargs)
	{
		return;
	}

	free((char*)pgargs->querootdir);
	free((char*)pgargs->quename);
	free((char*)pgargs->usrnam);
	free((char*)pgargs->grpnam);
	free((char*)pgargs->eworkdir);
	free((char*)pgargs->execpath);
	free((char*)pgargs->execargs);
	free((char*)pgargs->metatext);
	free((char*)pgargs->textdata);
	free((char*)pgargs->inputfile);
	free((char*)pgargs->soutpath);
	free((char*)pgargs->serrpath);
	free((char*)pgargs->printmethod);

	bzero(pgargs, sizeof(*pgargs));
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

#define RESET_STR(org_, MEMBER_) \
	char* c = strdup(org_); \
	if (! c) { \
		snprintf(message, sizeof(message), "'%c': mem alloc", opt); \
		jumppos = __LINE__; \
		goto EXIT_LABEL; \
	} \
	free((char*)pgargs->MEMBER_); \
	pgargs->MEMBER_ = c;

int sfqc_parse_program_args(int argc, char** argv, const char* optstring,
	sfq_bool use_rest, struct sfqc_program_args* pgargs)
{
	int irc = 1;
	int opt = 0;

	int jumppos = -1;
	char message[128] = "";

	assert(pgargs);

/* */
	bzero(pgargs, sizeof(*pgargs));

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

				pgargs->filesize_limit = ul;

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

				pgargs->payloadsize_limit = ul;

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

				pgargs->boota_proc_num = (ushort)ul;

				break;
			}

			case 'N': { RESET_STR(optarg, quename);		break; } // QUEUE 名
			case 'D': { RESET_STR(optarg, querootdir);	break; } // QUEUE ディレクトリ
			case 'U': { RESET_STR(optarg, usrnam);		break; } // QUEUE ユーザ
			case 'G': { RESET_STR(optarg, grpnam);		break; } // QUEUE グループ
			case 'w': { RESET_STR(optarg, eworkdir);	break; } // 作業ディレクトリ
			case 'x': { RESET_STR(optarg, execpath);	break; } // exec() パス
			case 'v': { RESET_STR(optarg, textdata);	break; } // データ# テキスト
			case 'f': { RESET_STR(optarg, inputfile);	break; } // データ# ファイル名
			case 'm': { RESET_STR(optarg, metatext);	break; } // メタ情報
			case 'p': { RESET_STR(optarg, printmethod);	break; } // pop, shift の出力方法

			// exec() 引数
			case 'a':
			{
				char* c = NULL;

				if (pgargs->execargs)
				{
					c = sfq_alloc_concat(pgargs->execargs, "\t", optarg);
				}
				else
				{
					c = strdup(optarg);
				}

				if (! c)
				{
					snprintf(message, sizeof(message), "'%c': mem alloc", opt);

					jumppos = __LINE__;
					goto EXIT_LABEL;
				}


				free((char*)pgargs->execargs);
				pgargs->execargs = c;

				break;
			}

			// 標準出力のリダイレクト先
			case 'o': { RESET_STR(optarg ? optarg : SFQ_DEFAULT_LOG, soutpath); break; }

			// 標準エラーのリダイレクト先
			case 'e': { RESET_STR(optarg ? optarg : SFQ_DEFAULT_LOG, serrpath); break; }

			case 'q':
			{
				pgargs->quiet = SFQ_true;
				break;
			}

			case 'r':
			{
				pgargs->reverse = SFQ_true;
				break;
			}

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			{
				pgargs->num1char = (opt - '0');
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

		sfqc_free_program_args(pgargs);

		return irc;
	}

	if (pgargs->filesize_limit == 0)
	{
		/* default 256MB */
		pgargs->filesize_limit = (256 * 1024 * 1024);
	}

	if (use_rest)
	{
		pgargs->commands = (const char**)&argv[optind];
		pgargs->command_num = argc - optind;
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

int sfqc_parse_printmethod(const char* arg, uint* printmethod_ptr)
{
	uint printmethod = SFQC_PRM_DEFAULT;

	if (! printmethod_ptr)
	{
		return 1;
	}

	if (arg)
	{
		char** params = NULL;
		char** pos = NULL;

		params = sfqc_split(sfq_stradup(arg), ',');
		if (params)
		{
			pos = params;
			while (*pos)
			{
				if (strcmp(*pos, "json") == 0)
				{
					printmethod |= SFQC_PRM_ASJSON;
				}
				else if (strcmp(*pos, "adda") == 0)
				{
					printmethod |= SFQC_PRM_ADD_ATTRIBUTE;
				}
				else if (strcmp(*pos, "http") == 0)
				{
					printmethod |= SFQC_PRM_HTTP_HEADER;
				}
				else if (strcmp(*pos, "pb64") == 0)
				{
					printmethod |= SFQC_PRM_PAYLOAD_BASE64;
				}
				else
				{
					/* ignore */
					fprintf(stderr, "%s(%d): unknown-option [%s] ignore\n",
						__FILE__, __LINE__, *pos);
				}

				pos++;
			}

			free(params);
			params = NULL;

		}
	}

	(*printmethod_ptr) = printmethod;

	return 0;
}

