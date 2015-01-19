#include "sfqc-lib.h"

#define IFEQ_DUP(name_, key_, val_) \
	\
	if (strcasecmp(#name_, (key_) ) == 0) { \
		if (! xd->pgargs.name_) \
		{ \
			const char* sd_ = strdup( (val_) ); \
			if (! sd_) { \
				goto EXIT_LABEL; \
			} \
			xd->pgargs.name_ = sd_; \
		} \
	}

static int set_pgargs(const char* arg_key, const char* val, struct sfqc_xinetd_data* xd)
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
			xd->payload_type = SFQ_PLT_CHARARRAY | SFQ_PLT_NULLTERM;
		}
		else if (strcasecmp(val, "binary") == 0)
		{
			xd->payload_type = SFQ_PLT_BINARY;
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

		xd->payload_len = ul;
	}

	rc = 0;

EXIT_LABEL:

	return rc;
}

int sfqc_xinetd_readdata(struct sfqc_xinetd_data* xd_ptr)
{
	int irc = 1;
	int rc = 1;

	char buff[8192];
	sfq_bool cont = SFQ_true;

	const char* pattern = "^\\s*([^:[:blank:]]+)\\s*:\\s*(\\S+)\\s*$";

	struct sfqc_xinetd_data xd;

	regex_t reg;
	regmatch_t matches[3];
	size_t nmatch = 0;

/* */
	bzero(&xd, sizeof(xd));

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
		sfqc_rtrim(buff);

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

					irc = set_pgargs(key, val, &xd);
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

	if (xd.payload_type)
	{
		if (xd.payload_len)
		{
			size_t iosize = 0;

			xd.payload = malloc(xd.payload_len + 1);
			if (! xd.payload)
			{
				goto EXIT_LABEL;
			}
			xd.payload[xd.payload_len] = '\0';

			iosize = fread(xd.payload, xd.payload_len, 1, stdin);
			if (iosize != 1)
			{
				goto EXIT_LABEL;
			}

			if (xd.payload_type & SFQ_PLT_CHARARRAY)
			{
				xd.payload_size = strlen((char*)xd.payload) + 1;
			}
			else
			{
				xd.payload_size = xd.payload_len;
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
		if (xd.payload_len)
		{
			goto EXIT_LABEL;
		}
	}

	(*xd_ptr) = xd;

	rc = 0;

EXIT_LABEL:

	regfree(&reg);

	if (rc != 0)
	{
		free(xd.payload);
		xd.payload = NULL;
	}

	return rc;
}

void sfqc_xinetd_fault(uint printmethod, int result_code, const char* message,
	sfq_bool quiet, const char* e_fn, int e_pos)
{
	if (! message)
	{
		return;
	}

	if (printmethod & SFQC_PRM_HTTP_HEADER)
	{
		sfqc_print_http_headers(SFQ_false, "text/plain; charset=UTF-8", strlen(message));

		sfqc_h_printf(SFQ_true, "result-code: %d" SFQC_CRLF, result_code);
		sfqc_h_printf(SFQ_true, "error-message: %s" SFQC_CRLF, message);

		printf(SFQC_CRLF);
		printf("%s" SFQC_CRLF, message);
	}
	else if (printmethod & SFQC_PRM_ADD_ATTRIBUTE)
	{
		sfqc_h_printf(SFQ_false, "result-code: %d" SFQC_CRLF, result_code);
		sfqc_h_printf(SFQ_false, "error-message: %s" SFQC_CRLF, message);

		printf(SFQC_CRLF);
		printf("%s" SFQC_CRLF, message);
	}
	else
	{
		if (! quiet)
		{
			fprintf(stderr, "%s(%d): %s\n", e_fn, e_pos, message);
		}
	}
}

