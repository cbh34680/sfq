#include "sfqc-lib.h"
#include "base64.h"

#include <jansson.h>

#define CRLF	"\r\n"

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

/*
static void chomp(char* p)
{
	char* pos = NULL;

	if (! p)
	{
		return;
	}

	pos = &p[strlen(p) - 1];

	while (pos != p)
	{
		if ((*pos) != '\n')
		{
			break;
		}

		(*pos) = '\0';

		pos--;
	}
}
*/

static void print_date(const char* key, time_t t)
{
	struct tm tmp;
	char dt[64];

	gmtime_r(&t, &tmp);
	strftime(dt, sizeof dt, "%a, %d %b %Y %H:%M:%S %Z", &tmp);

	printf("%s: %s" CRLF, key, dt);
}

static void print_http_headers(sfq_bool exist, const char* content_type, size_t content_length)
{
	time_t now = time(NULL);

	printf("HTTP/1.0 ");

	if (exist)
	{
		printf("200 OK" CRLF);
	}
	else
	{
		printf("204 No Content" CRLF);
	}

	printf("Content-Type: %s" CRLF, content_type);
	printf("Content-Length: %zu" CRLF, content_length);

	print_date("Date", now);
	print_date("Expires", now + 10);
/*
	printf("Date: %s" CRLF, dt);
	printf("Expires: -1" CRLF);
	printf("Cache-Control: no-cache" CRLF);
*/
	printf("Connection: close" CRLF);
}

/* ------------------------------------------------------------------------------- */
// outputtype=plain/text

static void h_printf(sfq_bool http, const char* org_format, ...)
{
	va_list arg;
	char* format = NULL;

	format = sfq_stradup(org_format);

	if (http)
	{
		printf("X-Sfq-");
		to_camelcase(format);
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvarargs"
	va_start(arg, format);
#pragma GCC diagnostic pop
	vprintf(format, arg);
	va_end(arg);
}

static void print_custom_headers(sfq_bool http, const struct sfq_value* val,
	size_t data_len, sfq_bool pbin, size_t pb64text_len)
{
	char uuid_s[36 + 1] = "";

/* */
	uuid_unparse(val->uuid, uuid_s);

	h_printf(http, "id: %zu\n", val->id);
	h_printf(http, "pushtime: %zu\n", val->pushtime);
	h_printf(http, "uuid: %s\n", uuid_s);

	if (val->execpath)
	{
		h_printf(http, "execpath: %s\n", val->execpath);
	}

	if (val->execargs)
	{
		h_printf(http, "execargs: %s\n", val->execargs);
	}

	if (val->metatext)
	{
		h_printf(http, "metatext: %s\n", val->metatext);
	}

	if (val->soutpath)
	{
		h_printf(http, "soutpath: %s\n", val->soutpath);
	}

	if (val->serrpath)
	{
		h_printf(http, "serrpath: %s\n", val->serrpath);
	}

	h_printf(http, "payload-length: %zu\n", data_len);
	h_printf(http, "payload-size-in-element: %zu\n", val->payload_size);

	if (val->payload)
	{
		h_printf(http, "payload-type: %s\n", pbin ? "binary" : "text");
	}

	h_printf(http, "payload-encoding: %s\n", pb64text_len ? "base64" : "none");

	if (pb64text_len)
	{
		h_printf(http, "payload-base64-length: %zu\n", pb64text_len);
	}
}

static void print_raw(uint printmethod, const struct sfq_value* val)
{
	char* message = NULL;
	int jumppos = 0;

	const sfq_byte* data = NULL;
	size_t data_len = 0;

	char* pb64text = NULL;
	size_t pb64text_len = 0;

	size_t iosize = 0;

	sfq_bool pbin = (val->payload_type & SFQ_PLT_BINARY);
	sfq_bool pb64 = (printmethod & SFQC_PRM_PAYLOAD_BASE64);
	sfq_bool adda = (printmethod & SFQC_PRM_ADD_ATTRIBUTE);
	sfq_bool http = (printmethod & SFQC_PRM_HTTP_HEADER);

	sfq_bool text_output = pb64 ? SFQ_true : (val->payload_type & SFQ_PLT_CHARARRAY);

/* */
	char uuid_s[36 + 1] = "";

/* */
	uuid_unparse(val->uuid, uuid_s);

	data = val->payload;
	data_len = sfq_payload_len(val);

	if (pb64)
	{
		pb64text = (char*)base64_encode(data, data_len, &pb64text_len);
		if (! pb64text)
		{
			message = "base64_encode";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}

		//chomp(pb64text);

		data = (sfq_byte*)pb64text;
		data_len = strlen(pb64text);
	}

	if (http)
	{
		const char* content_type = text_output
			? "text/plain; charset=UTF-8" : "application/octed-stream";

		print_http_headers(SFQ_true, content_type, data_len);

		if (! text_output)
		{
			printf("Content-Disposition: attachment; filename=\"%s.dat\"" CRLF, uuid_s);
		}
	}

	if (adda)
	{
		print_custom_headers(http, val, data_len, pbin, pb64text_len);
	}

	if (http || adda)
	{
		printf(CRLF);
	}

	if (data_len)
	{
		iosize = fwrite(data, data_len, 1, stdout);
		if (! iosize)
		{
			message = "fwrite";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}
	}

EXIT_LABEL:

	free(pb64text);
	pb64text = NULL;

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}
}

/* ------------------------------------------------------------------------------- */
// outputtype=json

static char* create_json_string(const struct sfq_value* val, sfq_bool pbin, sfq_bool pb64, size_t pb64text_len,
	sfq_bool adda, const sfq_byte* data, size_t data_len)
{
	int irc = 1;

	char* message = NULL;
	int jumppos = 0;

	json_t* json = NULL;
	char* dumps = NULL;

	char uuid_s[36 + 1] = "";

/* */
	uuid_unparse(val->uuid, uuid_s);


/* build json object */
	json = json_object();

	if (adda)
	{
		json_object_set_new(json, "id", json_integer(val->id));
		json_object_set_new(json, "uuid", json_string(uuid_s));
		json_object_set_new(json, "pushtime", json_integer(val->pushtime));

		if (val->execpath)
		{
			json_object_set_new(json, "execpath", json_string(val->execpath));
		}

		if (val->execargs)
		{
			json_object_set_new(json, "execargs", json_string(val->execargs));
		}

		if (val->metatext)
		{
			json_object_set_new(json, "metatext", json_string(val->metatext));
		}

		if (val->soutpath)
		{
			json_object_set_new(json, "soutpath", json_string(val->soutpath));
		}

		if (val->serrpath)
		{
			json_object_set_new(json, "serrpath", json_string(val->serrpath));
		}

		json_object_set_new(json, "payload-length", json_integer(data_len));
		json_object_set_new(json, "payload-size-in-element", json_integer(val->payload_size));

		if (val->payload)
		{
			json_object_set_new(json, "payload-type", json_string(pbin ? "binary" : "text"));
		}

		json_object_set_new(json, "payload-encoding",
			json_string(pb64text_len ? "base64" : "none"));

		if (pb64text_len)
		{
			json_object_set_new(json, "payload-base64-length", json_integer(pb64text_len));
		}
	}

	if (data)
	{
		json_object_set_new(json, "payload", json_stringn((char*)data, data_len));
	}

	dumps = json_dumps(json, 0);
	if (! dumps)
	{
		message = "json_dumps";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = 0;

EXIT_LABEL:

	if (irc != 0)
	{
		free(dumps);
		dumps = NULL;
	}

	json_decref(json);
	json = NULL;

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

	return dumps;
}

static void print_by_json(uint printmethod, const struct sfq_value* val)
{
	char* message = NULL;
	int jumppos = 0;

	const sfq_byte* data = NULL;
	size_t data_len = 0;

	char* pb64text = NULL;
	size_t pb64text_len = 0;

	char* jsontext = NULL;

	sfq_bool pbin = (val->payload_type & SFQ_PLT_BINARY);
	sfq_bool pb64 = (printmethod & SFQC_PRM_PAYLOAD_BASE64);
	sfq_bool adda = (printmethod & SFQC_PRM_ADD_ATTRIBUTE);
	sfq_bool http = (printmethod & SFQC_PRM_HTTP_HEADER);

/* */
	data = val->payload;
	data_len = sfq_payload_len(val);

	if (pbin || pb64)
	{
		pb64text = (char*)base64_encode(data, data_len, &pb64text_len);
		if (! pb64text)
		{
			message = "base64_encode";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}

		//chomp(pb64text);

		data = (sfq_byte*)pb64text;
		data_len = strlen(pb64text);
	}

	jsontext = create_json_string(val, pbin, pb64, pb64text_len, adda, data, data_len);
	if (! jsontext)
	{
		message = "create_json_string";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (http)
	{
		print_http_headers(SFQ_true, "application/json", strlen(jsontext));
	}

	if (adda)
	{
		print_custom_headers(http, val, data_len, pbin, pb64text_len);
	}

	if (http || adda)
	{
		printf(CRLF);
	}

	printf("%s" CRLF, jsontext);

EXIT_LABEL:

	free(pb64text);
	pb64text = NULL;

	free(jsontext);
	jsontext = NULL;

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}
}

/* ------------------------------------------------------------------------------- */

static int parse_printmethod(const char* arg, uint* printmethod_ptr)
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

int sfqc_takeout(int argc, char** argv, sfq_takeoutfunc_t takeoutfunc)
{
	int irc = 0;
	char* message = NULL;
	int jumppos = 0;

	uint printmethod = 0;

/* */
	struct sfqc_program_args pgargs;
	struct sfq_value val;
	struct sfq_value pval;

SFQC_MAIN_ENTER

	bzero(&pgargs, sizeof(pgargs));
	bzero(&val, sizeof(val));
	bzero(&pval, sizeof(pval));

/* */
	irc = sfqc_parse_program_args(argc, argv, "D:N:p:q", SFQ_false, &pgargs);
	if (irc != 0)
	{
		message = "parse_program_args: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = parse_printmethod(pgargs.printmethod, &printmethod);
	if (irc != 0)
	{
		message = "parse_printmethod";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = takeoutfunc(pgargs.querootdir, pgargs.quename, &val);
	if (irc != 0)
	{
		message = "takeout";

		switch (irc)
		{
			case SFQ_RC_W_NOELEMENT:
			{
				message = "element does not exist in the queue";
				break;
			}

			case SFQ_RC_W_TAKEOUT_STOPPED:
			{
				message = "queue is stopped retrieval";
				break;
			}

		}

		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

#if 0
SFQ_DEBUG_BUILD
	irc = sfq_alloc_print_value(&val, &pval);
	if (irc != SFQ_RC_SUCCESS)
	{
		message = "sfq_alloc_print_value";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	char uuid_s[36 + 1] = "";
	uuid_unparse(val.uuid, uuid_s);

	puts("=");
	printf("%zu\n%zu\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
		pval.id, pval.pushtime, uuid_s, pval.execpath, pval.execargs, pval.metatext,
		pval.soutpath, pval.serrpath,
		pval.payload);
	puts("=");
#endif

	if (printmethod & SFQC_PRM_ASJSON)
	{
		print_by_json(printmethod, &val);
	}
	else
	{
		print_raw(printmethod, &val);
	}

EXIT_LABEL:

	sfq_free_value(&val);
	sfq_free_value(&pval);

	if (printmethod & SFQC_PRM_HTTP_HEADER)
	{
		print_http_headers(SFQ_false, "text/plain; charset=UTF-8", strlen(message));

		printf(CRLF);
		printf("%s" CRLF, message);
	}
	else
	{
		if (! pgargs.quiet)
		{
			if (message)
			{
				fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
			}
		}
	}

	sfqc_free_program_args(&pgargs);

SFQC_MAIN_LEAVE

	return irc;
}

