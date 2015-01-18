#include "sfqc-lib.h"

static struct sfqc_program_args pgargs;
static sfq_byte* mem = NULL;
static size_t memsize = 0;

static void release_heap()
{
	free(mem);
	mem = NULL;

	memsize = 0;

	sfqc_free_program_args(&pgargs);
}

static void rtrim(char* p)
{
	char* pos = &p[strlen(p)];

	do
	{
		pos--;

		if (isspace(*pos))
		{
			(*pos) = '\0';
		}
		else
		{
			break;
		}
	}
	while (pos != p);
}

struct push_attr
{
	payload_type_t payload_type;
	size_t payload_len;
};

#define IFEQ_DUP(name_, key_, val_) \
	\
	if (strcasecmp(#name_, (key_) ) == 0) { \
		if (! pgargs.name_) \
		{ \
			const char* sd_ = strdup( (val_) ); \
			if (! sd_) { \
				goto EXIT_LABEL; \
			} \
			pgargs.name_ = sd_; \
		} \
	}

static int set_pgargs(const char* arg_key, const char* val, struct push_attr* pattr)
{
	int rc = 1;

	char* key = NULL;
	char* pos = NULL;

	key = sfq_stradup(arg_key);
	if (! key)
	{
		goto EXIT_LABEL;
	}

	pos = key;
	while (*pos)
	{
		if ( (*pos) == '_' )
		{
			(*pos) = '-';
		}

		pos++;
	}

	IFEQ_DUP(querootdir, key, val)
	else
	IFEQ_DUP(quename, key, val)
	else
	IFEQ_DUP(eworkdir, key, val)
	else
	IFEQ_DUP(execpath, key, val)
	else
	IFEQ_DUP(execargs, key, val)
	else
	IFEQ_DUP(metatext, key, val)
	else
	IFEQ_DUP(soutpath, key, val)
	else
	IFEQ_DUP(serrpath, key, val)
	else
	if (strcasecmp(key, "payload-type") == 0)
	{
		if (strcasecmp(val, "text") == 0)
		{
			pattr->payload_type = SFQ_PLT_CHARARRAY | SFQ_PLT_NULLTERM;
		}
		else if (strcasecmp(val, "binary") == 0)
		{
			pattr->payload_type = SFQ_PLT_BINARY;
		}
		else
		{
			goto EXIT_LABEL;
		}
	}
	else
	if (strcasecmp(key, "payload-length") == 0)
	{
		char* e = NULL;
		unsigned long ul = strtoul(val, &e, 0);

		if (*e)
		{
			goto EXIT_LABEL;
		}

		pattr->payload_len = ul;
	}

	rc = 0;

EXIT_LABEL:

	return rc;
}

static int readstdin_(struct push_attr* pattr)
{
	int irc = 1;
	int rc = 1;

	char buff[8192];
	sfq_bool cont = SFQ_true;

	const char* pattern = "^\\s*([^:[:blank:]]+)\\s*:\\s*(\\S+)\\s*$";

	regex_t reg;
	regmatch_t matches[3];
	size_t nmatch = 0;

	sfq_byte* payload = NULL;

/* */
	bzero(&reg, sizeof(reg));
	bzero(&matches, sizeof(matches));

	nmatch = sizeof(matches) / sizeof(matches[0]);

	irc = regcomp(&reg, pattern, REG_EXTENDED | REG_NEWLINE);
	if (irc != 0)
	{
		goto EXIT_LABEL;
	}

/* */
	while (cont && fgets(buff, sizeof(buff), stdin))
	{
		rtrim(buff);

		if (buff[0] == '\0')
		{
			// 空行(=ヘッダ終了) なので body を読み込む

			cont = SFQ_false;
		}
		else
		{
			irc = regexec(&reg, buff, nmatch, matches, 0);
			if (irc == 0)
			{
				int so1 = matches[1].rm_so;
				int eo1 = matches[1].rm_eo;

				int so2 = matches[2].rm_so;
				int eo2 = matches[2].rm_eo;

				if ((so1 >= 0) && (eo1 >= 0) && (so2 >= 0) && (eo2 >= 0))
				{
					const char* key = sfq_strandup(&buff[so1], (eo1 - so1));
					const char* val = sfq_strandup(&buff[so2], (eo2 - so2));

					if ((! key) || (! val))
					{
						goto EXIT_LABEL;
					}

					irc = set_pgargs(key, val, pattr);
					if (irc != 0)
					{
						goto EXIT_LABEL;
					}
				}
			}
		}
	}

	if (feof(stdin))
	{
		goto EXIT_LABEL;
	}

	if (pattr->payload_type)
	{
		if (pattr->payload_len)
		{
			size_t iosize = 0;

			payload = malloc(pattr->payload_len + 1);
			if (! payload)
			{
				goto EXIT_LABEL;
			}
			payload[pattr->payload_len] = '\0';

			iosize = fread(payload, pattr->payload_len, 1, stdin);
			if (iosize != 1)
			{
				goto EXIT_LABEL;
			}

			mem = payload;

			if (pattr->payload_type & SFQ_PLT_CHARARRAY)
			{
				memsize = strlen((char*)mem) + 1;
			}
			else
			{
				memsize = pattr->payload_len;
			}
		}
		else
		{
/*
body 部全部を mem, memsize に設定
*/
		}
	}
	else
	{
		if (pattr->payload_len)
		{
			goto EXIT_LABEL;
		}
	}

	rc = 0;

EXIT_LABEL:


	regfree(&reg);

	if (rc != 0)
	{
		free(payload);
		payload = NULL;
	}

	return rc;
}

int main(int argc, char** argv)
{
	int irc = SFQ_RC_UNKNOWN;
	char* message = NULL;
	int jumppos = 0;
	struct push_attr pattr;

	uuid_t uuid;
	uint printmethod = 0;

/* */

SFQC_MAIN_ENTER

	bzero(&pgargs, sizeof(pgargs));
	bzero(&pattr, sizeof(pattr));
	uuid_clear(uuid);

	atexit(release_heap);

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:w:o:e:f:x:a:m:p:q", SFQ_false, &pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	sfq_set_print(pgargs.quiet ? SFQ_false : SFQ_true);

	irc = sfqc_parse_printmethod(pgargs.printmethod, &printmethod);
	if (irc != 0)
	{
		message = "sfqc_parse_printmethod";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

/* read from stdin */

	irc = readstdin_(&pattr);

	if (irc != 0)
	{
		message = "can't read stdin";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (pattr.payload_type & SFQ_PLT_CHARARRAY)
	{
		irc = sfq_push_text(pgargs.querootdir, pgargs.quename,
			pgargs.eworkdir, pgargs.execpath, pgargs.execargs,
			pgargs.metatext, pgargs.soutpath, pgargs.serrpath,
			uuid,
			(char*)mem);
	}
	else
	{
		irc = sfq_push_binary(pgargs.querootdir, pgargs.quename,
			pgargs.eworkdir, pgargs.execpath, pgargs.execargs,
			pgargs.metatext, pgargs.soutpath, pgargs.serrpath,
			uuid,
			mem, memsize);
	}

	if (irc != SFQ_RC_SUCCESS)
	{
		switch (irc)
		{
			case SFQ_RC_W_NOSPACE:
			{
				message = "no free space in the queue";
				break;
			}
			case SFQ_RC_W_ACCEPT_STOPPED:
			{
				message = "queue has stop accepting";
				break;
			}
			default:
			{
				message = "sfq_push_binary";
				break;
			}
		}

		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	sfqc_push_success(printmethod, uuid);

EXIT_LABEL:

	free(mem);
	mem = NULL;

	sfqc_push_fault(printmethod, irc, message, pgargs.quiet, __FILE__, jumppos);

	sfqc_free_program_args(&pgargs);

SFQC_MAIN_LEAVE

	return irc;
}

