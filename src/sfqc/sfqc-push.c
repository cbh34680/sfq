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

