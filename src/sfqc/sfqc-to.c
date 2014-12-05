#include "sfqc-lib.h"
#include "base64.h"

#include <jansson.h>

static char** split(char* params_str, char c_delim)
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

		params = split(sfq_stradup(arg), ',');
		if (params)
		{
			pos = params;
			while (*pos)
			{
				if (strcmp(*pos, "json") == 0)
				{
					printmethod |= SFQC_PRM_ASJSON;
				}
				else if (strcmp(*pos, "witha") == 0)
				{
					printmethod |= SFQC_PRM_WITH_ATTR;
				}
				else if (strcmp(*pos, "b64p") == 0)
				{
					printmethod |= SFQC_PRM_PAYLOAD_B64;
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

static int print_notjson(const struct sfq_value* val, uint printmethod)
{
	int irc = 1;

	char* message = NULL;
	int jumppos = 0;

	sfq_byte* b64mem = NULL;
	size_t b64mem_size = 0;

	char uuid_s[36 + 1] = "";

	uuid_unparse(val->uuid, uuid_s);

	if (val->payload && (printmethod & SFQC_PRM_PAYLOAD_B64))
	{
/*
バイナリデータを base64 エンコード
*/
		b64mem = base64_encode(val->payload, val->payload_size, &b64mem_size);
		if (! b64mem)
		{
			message = "base64_encode(b64len)";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}
	}

	if (printmethod & SFQC_PRM_WITH_ATTR)
	{
/*
属性情報を出力
*/
		printf("id: %zu\n", val->id);
		printf("pushtime: %zu\n", val->pushtime);
		printf("uuid: %s\n", uuid_s);

		if (val->execpath)
		{
			printf("execpath: %s\n", val->execpath);
		}

		if (val->execargs)
		{
			printf("execargs: %s\n", val->execargs);
		}

		if (val->metadata)
		{
			printf("metadata: %s\n", val->metadata);
		}

		if (val->soutpath)
		{
			printf("soutpath: %s\n", val->soutpath);
		}

		if (val->serrpath)
		{
			printf("serrpath: %s\n", val->serrpath);
		}

		if (val->payload)
		{
			printf("payload-type: %s\n",
				(val->payload_type & SFQ_PLT_CHARARRAY) ? "text" : "binary");

/*
payload を出力するときに '\0' は含まないので NULLTERM の場合は -1
*/
			printf("payload-size: %zu\n",
				(val->payload_type & SFQ_PLT_NULLTERM)
					? (val->payload_size - 1) : val->payload_size);
		}

		if (b64mem)
		{
			printf("payload-output-encode: base64\n");
			printf("base64-payload-size: %zu\n", b64mem_size);
		}

		printf("\n");
	}

	if (val->payload)
	{
		size_t iosize = 0;

		unsigned char* mem = NULL;
		size_t mem_size = 0;

		assert(val->payload_size);

		if (val->payload_type & SFQ_PLT_CHARARRAY)
		{
			mem = val->payload;
			mem_size = val->payload_size;

			if (val->payload_type & SFQ_PLT_NULLTERM)
			{
				mem_size--;
			}
		}
		else
		{
			if (b64mem)
			{
				mem = b64mem;
				mem_size = b64mem_size;
			}
			else
			{
				mem = val->payload;
				mem_size = val->payload_size;
			}
		}

		iosize = fwrite(mem, mem_size, 1, stdout);
		if (iosize != 1)
		{
			message = "fwrite(mem)";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}
	}

	irc = 0;

EXIT_LABEL:

	free(b64mem);
	b64mem = NULL;

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

	return irc;
}

static int print_json(const struct sfq_value* val, uint printmethod)
{
	int irc = 1;

	char* message = NULL;
	int jumppos = 0;

	sfq_byte* b64mem = NULL;
	size_t b64mem_size = 0;

	json_t* json = NULL;
	char* dumps = NULL;

	char uuid_s[36 + 1] = "";

/* */
	uuid_unparse(val->uuid, uuid_s);

	if (val->payload && (val->payload_type & SFQ_PLT_BINARY))
	{
/*
バイナリデータを base64 エンコード
*/
		b64mem = base64_encode(val->payload, val->payload_size, &b64mem_size);
		if (! b64mem)
		{
			message = "base64_encode(b64len)";
			jumppos = __LINE__;
			goto EXIT_LABEL;
		}
	}

/* build json object */
	json = json_object();

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

	if (val->payload)
	{
		json_object_set_new(json, "payload-type", json_string(
			(val->payload_type & SFQ_PLT_CHARARRAY) ? "text" : "binary"));

		json_object_set_new(json, "payload-size", json_integer(val->payload_size));
	}

	json_object_set_new(json, "payload-output-encode", json_string(b64mem ? "base64" : "none"));

	if (b64mem)
	{
		json_object_set_new(json, "base64-payload-size", json_integer(b64mem_size));
	}

	if (val->payload)
	{
		unsigned char* mem = NULL;
		size_t mem_size = 0;

		assert(val->payload_size);

		if (val->payload_type & SFQ_PLT_CHARARRAY)
		{
			mem = val->payload;
			mem_size = val->payload_size;

			if (val->payload_type & SFQ_PLT_NULLTERM)
			{
				mem_size--;
			}
		}
		else
		{
			assert(b64mem);

			mem = b64mem;
			mem_size = b64mem_size;
		}

		json_object_set_new(json, "payload", json_stringn((const char*)mem, mem_size));
	}

	dumps = json_dumps(json, 0);
	puts(dumps);

	irc = 0;

EXIT_LABEL:

	free(dumps);
	dumps = NULL;

	free(b64mem);
	b64mem = NULL;

	json_decref(json);
	json = NULL;

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

	return irc;
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
	irc = sfqc_get_init_option(argc, argv, "D:N:p:", 0, &opt);
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

#ifdef SFQ_DEBUG_BUILD
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
/* json */
		irc = print_json(&val, printmethod);
	}
	else
	{
		irc = print_notjson(&val, printmethod);
	}

EXIT_LABEL:

	sfq_free_value(&val);
	sfq_free_value(&pval);
	sfqc_free_init_option(&opt);

	if (message)
	{
		fprintf(stderr, "%s(%d): %s\n", __FILE__, jumppos, message);
	}

SFQC_MAIN_FINALIZE

	return irc;
}

