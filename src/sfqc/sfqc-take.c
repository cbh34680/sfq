#include "sfqc-lib.h"
#include "base64.h"

#include <jansson.h>

static void print_custom_headers(sfq_bool http, const struct sfq_value* val,
	size_t data_len, sfq_bool pbin, size_t pb64text_len)
{
	char uuid_s[36 + 1] = "";

/* */
	uuid_unparse(val->uuid, uuid_s);

/* */
	sfqc_h_printf(http, "result-code: 0" SFQC_CRLF);
	sfqc_h_printf(http, "id: %zu" SFQC_CRLF, val->id);
	sfqc_h_printf(http, "pushtime: %zu" SFQC_CRLF, val->pushtime);
	sfqc_h_printf(http, "uuid: %s" SFQC_CRLF, uuid_s);

	if (val->querootdir)
	{
		sfqc_h_printf(http, "querootdir: %s" SFQC_CRLF, val->querootdir);
	}

	if (val->quename)
	{
		sfqc_h_printf(http, "quename: %s" SFQC_CRLF, val->quename);
	}

	if (val->eworkdir)
	{
		sfqc_h_printf(http, "eworkdir: %s" SFQC_CRLF, val->eworkdir);
	}

	if (val->execpath)
	{
		sfqc_h_printf(http, "execpath: %s" SFQC_CRLF, val->execpath);
	}

	if (val->execargs)
	{
		sfqc_h_printf(http, "execargs: %s" SFQC_CRLF, val->execargs);
	}

	if (val->metatext)
	{
		sfqc_h_printf(http, "metatext: %s" SFQC_CRLF, val->metatext);
	}

	if (val->soutpath)
	{
		sfqc_h_printf(http, "soutpath: %s" SFQC_CRLF, val->soutpath);
	}

	if (val->serrpath)
	{
		sfqc_h_printf(http, "serrpath: %s" SFQC_CRLF, val->serrpath);
	}

	sfqc_h_printf(http, "payload-length: %zu" SFQC_CRLF, data_len);
	sfqc_h_printf(http, "payload-size-in-element: %zu" SFQC_CRLF, val->payload_size);

	if (val->payload)
	{
		sfqc_h_printf(http, "payload-type: %s" SFQC_CRLF, pbin ? "binary" : "text");
	}

	sfqc_h_printf(http, "payload-encoding: %s" SFQC_CRLF, pb64text_len ? "base64" : "none");

	if (pb64text_len)
	{
		sfqc_h_printf(http, "payload-base64-length: %zu" SFQC_CRLF, pb64text_len);
	}
}

static void print_raw(uint printmethod, const struct sfq_value* val)
{
	const char* message = NULL;

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

		sfqc_print_http_headers(SFQ_true, content_type, data_len);

		if (! text_output)
		{
			printf("Content-Disposition: attachment; filename=\"%s.dat\"" SFQC_CRLF, uuid_s);
		}
	}

	if (adda)
	{
		print_custom_headers(http, val, data_len, pbin, pb64text_len);
	}

	if (http || adda)
	{
		printf(SFQC_CRLF);
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

static char* create_json_string(const struct sfq_value* val,
	sfq_bool pbin, sfq_bool pb64, size_t pb64text_len,
	sfq_bool adda, const sfq_byte* data, size_t data_len)
{
	int irc = 1;

	const char* message = NULL;
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
		json_object_set_new(json, "result-code", json_string("ok"));
		json_object_set_new(json, "id", json_integer(val->id));
		json_object_set_new(json, "uuid", json_string(uuid_s));
		json_object_set_new(json, "pushtime", json_integer(val->pushtime));

		if (val->querootdir)
		{
			json_object_set_new(json, "querootdir", json_string(val->querootdir));
		}

		if (val->quename)
		{
			json_object_set_new(json, "quename", json_string(val->quename));
		}

		if (val->eworkdir)
		{
			json_object_set_new(json, "eworkdir", json_string(val->eworkdir));
		}

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
#if 0
/* jansson 2.7 */
		json_object_set_new(json, "payload", json_stringn((char*)data, data_len));
#else
		char* tmpValue = alloca(data_len + 1);
		memcpy(tmpValue, data, data_len);
		tmpValue[data_len] = '\0';

		json_object_set_new(json, "payload", json_string(tmpValue));
#endif
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

static void print_json(uint printmethod, const struct sfq_value* val)
{
	const char* message = NULL;
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
		sfqc_print_http_headers(SFQ_true, "application/json", strlen(jsontext));
	}

	if (adda)
	{
		print_custom_headers(http, val, data_len, pbin, pb64text_len);
	}

	if (http || adda)
	{
		printf(SFQC_CRLF);
	}

	printf("%s" SFQC_CRLF, jsontext);

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

void sfqc_take_success(uint printmethod, const struct sfq_value* val)
{
	if (printmethod & SFQC_PRM_ASJSON)
	{
		print_json(printmethod, val);
	}
	else
	{
		print_raw(printmethod, val);
	}
}

