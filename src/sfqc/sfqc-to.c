#include "sfqc-lib.h"
#include "base64.h"

#include <jansson.h>

#define CRLF	"\r\n"

/* ------------------------------------------------------------------------------- */
// outputtype=json

static char* create_json_string(const struct sfq_value* val, bool hdrc,
	const sfq_byte* data, size_t data_len)
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

	if (hdrc)
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

		if (val->metadata)
		{
			json_object_set_new(json, "metadata", json_string(val->metadata));
		}

		if (val->soutpath)
		{
			json_object_set_new(json, "soutpath", json_string(val->soutpath));
		}

		if (val->serrpath)
		{
			json_object_set_new(json, "serrpath", json_string(val->serrpath));
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

	bool pbin = (val->payload_type & SFQ_PLT_BINARY);
	bool pb64 = (printmethod & SFQC_PRM_PAYLOAD_BASE64);
	bool hdrc = (printmethod & SFQC_PRM_HEADER_CUSTOM);
	bool hdrh = (printmethod & SFQC_PRM_HEADER_HTTP);

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

		data = (sfq_byte*)pb64text;
		data_len = pb64text_len;
	}

	jsontext = create_json_string(val, hdrc, data, data_len);
	if (! jsontext)
	{
		message = "create_json_string";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (hdrh)
	{
		printf("HTTP/1.0 200 OK" CRLF);
		printf("Content-Type: application/json" CRLF);
		printf("Content-Length: %zu" CRLF, strlen(jsontext));
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
// outputtype=plain/text

static void h_printf(bool hdrh, const char* org_format, ...)
{
	va_list arg;
	char* format = NULL;

	format = sfq_stradup(org_format);

	if (hdrh)
	{
		printf("X-Sfq-");

		char* pos = format;
		bool next_upper = true;

		while (*pos)
		{
			if (((*pos) == ' ') || ((*pos) == ':'))
			{
				break;
			}

			if ((*pos) == '-')
			{
				next_upper = true;
			}
			else
			{
				if (next_upper)
				{
					(*pos) = toupper(*pos);
					next_upper = false;
				}
			}

			pos++;
		}
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvarargs"
	va_start(arg, format);
#pragma GCC diagnostic pop
	vprintf(format, arg);
	va_end(arg);
}

static void print_by_text(uint printmethod, const struct sfq_value* val)
{
	char* message = NULL;
	int jumppos = 0;

	const sfq_byte* data = NULL;
	size_t data_len = 0;

	char* pb64text = NULL;
	size_t pb64text_len = 0;

	size_t iosize = 0;

	bool pb64 = (printmethod & SFQC_PRM_PAYLOAD_BASE64);
	bool hdrc = (printmethod & SFQC_PRM_HEADER_CUSTOM);
	bool hdrh = (printmethod & SFQC_PRM_HEADER_HTTP);

	bool is_text = pb64 ? true : (val->payload_type & SFQ_PLT_CHARARRAY);

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

		data = (sfq_byte*)pb64text;
		data_len = pb64text_len;
	}

	if (hdrh)
	{
		printf("HTTP/1.0 200 OK" CRLF);
		printf("Content-Type: %s" CRLF, is_text ? "text/plain" : "application/octed-stream");
		printf("Content-Length: %zu" CRLF, data_len);
	}

	if (hdrc)
	{
		h_printf(hdrh, "id: %zu\n", val->id);
		h_printf(hdrh, "pushtime: %zu\n", val->pushtime);
		h_printf(hdrh, "uuid: %s\n", uuid_s);

		if (val->execpath)
		{
			h_printf(hdrh, "execpath: %s\n", val->execpath);
		}

		if (val->execargs)
		{
			h_printf(hdrh, "execargs: %s\n", val->execargs);
		}

		if (val->metadata)
		{
			h_printf(hdrh, "metadata: %s\n", val->metadata);
		}

		if (val->soutpath)
		{
			h_printf(hdrh, "soutpath: %s\n", val->soutpath);
		}

		if (val->serrpath)
		{
			h_printf(hdrh, "serrpath: %s\n", val->serrpath);
		}
	}

	if (hdrh || hdrc)
	{
		printf(CRLF);
	}

	iosize = fwrite(data, data_len, 1, stdout);
	if (! iosize)
	{
		message = "fwrite";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (is_text)
	{
 		if (pb64)
		{
/*
base64 のときは data の最後が改行文字なのでここでは出力しない
*/
		}
		else
		{
			printf(CRLF);
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
				else if (strcmp(*pos, "hdrc") == 0)
				{
					printmethod |= SFQC_PRM_HEADER_CUSTOM;
				}
				else if (strcmp(*pos, "hdrh") == 0)
				{
					printmethod |= SFQC_PRM_HEADER_HTTP;
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
	struct sfqc_init_option opt;
	struct sfq_value val;
	struct sfq_value pval;

SFQC_MAIN_INITIALIZE

	bzero(&opt, sizeof(opt));
	bzero(&val, sizeof(val));
	bzero(&pval, sizeof(pval));

/* */
	irc = sfqc_get_init_option(argc, argv, "D:N:p:q", 0, &opt);
	if (irc != 0)
	{
		message = "get_init_option: parse error";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	irc = takeoutfunc(opt.querootdir, opt.quename, &val);
	if (irc != 0)
	{
		message = (irc == SFQ_RC_NO_ELEMENT) ? "element does not exist in the queue" : "takeout";
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
		pval.id, pval.pushtime, uuid_s, pval.execpath, pval.execargs, pval.metadata,
		pval.soutpath, pval.serrpath,
		pval.payload);
	puts("=");
#endif

	irc = parse_printmethod(opt.printmethod, &printmethod);
	if (irc != 0)
	{
		message = "parse_printmethod";
		jumppos = __LINE__;
		goto EXIT_LABEL;
	}

	if (printmethod & SFQC_PRM_ASJSON)
	{
		print_by_json(printmethod, &val);
	}
	else
	{
		print_by_text(printmethod, &val);
	}

EXIT_LABEL:

	sfq_free_value(&val);
	sfq_free_value(&pval);

	if (! opt.quiet)
	{
		if (message)
		{
			fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
		}
	}

	sfqc_free_init_option(&opt);

SFQC_MAIN_FINALIZE

	return irc;
}

