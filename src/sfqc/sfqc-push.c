#include "sfqc-lib.h"

void sfqc_push_success(uint printmethod, uuid_t uuid)
{
	char uuid_s[36 + 1] = "";

	if (! uuid_is_null(uuid))
	{
		uuid_unparse(uuid, uuid_s);
	}

	if (printmethod & SFQC_PRM_HTTP_HEADER)
	{
		sfqc_print_http_headers(SFQ_true, "text/plain; charset=UTF-8", strlen(uuid_s));

		sfqc_h_printf(SFQ_true, "result-code: 0" SFQC_CRLF);
		sfqc_h_printf(SFQ_true, "uuid: %s" SFQC_CRLF, uuid_s);

		printf(SFQC_CRLF);
	}
	else if (printmethod & SFQC_PRM_ADD_ATTRIBUTE)
	{
		sfqc_h_printf(SFQ_false, "result-code: 0" SFQC_CRLF);
		sfqc_h_printf(SFQ_false, "uuid: %s" SFQC_CRLF, uuid_s);

		printf(SFQC_CRLF);
	}

	printf("%s" SFQC_CRLF, uuid_s);
}

void sfqc_push_fault(uint printmethod, int result_code, const char* message,
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

