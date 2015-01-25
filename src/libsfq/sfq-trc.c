#include "sfq-lib.h"

#ifdef SFQ_TRACE_FILE

static FILE* g_trc_fp = NULL;
static int g_indent = 0;

static void trc_println(const char* format, ...)
{
	va_list ap;
	int i = 0;

	struct tm tm_tmp;
	time_t now;
	char nowstr[32] = "";

	if (! g_trc_fp)
	{
		return;
	}

	now = time(NULL);
	localtime_r(&now, &tm_tmp);
	strftime(nowstr, sizeof(nowstr), "%Y-%m-%d %H:%M:%S ", &tm_tmp);
	fprintf(g_trc_fp, nowstr);

	for (i=0; i<g_indent; i++)
	{
		fprintf(g_trc_fp, "\t");
	}

	va_start(ap, format);
	vfprintf(g_trc_fp, format, ap);
	va_end(ap);

	fprintf(g_trc_fp, "\n");
}

void sfq_trc_close()
{
	if (! g_trc_fp)
	{
		return;
	}

	fprintf(g_trc_fp, "--\n");
	fclose(g_trc_fp);
	g_trc_fp = NULL;

	g_indent = 0;
}

void sfq_trc_open()
{
	if (! g_trc_fp)
	{
		g_trc_fp = fopen(SFQ_TRACE_FILE, "at");

		chmod(SFQ_TRACE_FILE, 0666);
	}
}

void sfq_trc_enter(const char* file, int line, const char* func)
{
	if (! g_trc_fp)
	{
		return;
	}

	trc_println("> %s(%d) [%s]", file, line, func);
	g_indent++;
}

void sfq_trc_leave(const char* file, int line, const char* func, int rc, const char* msg)
{
	if (! g_trc_fp)
	{
		return;
	}

	g_indent--;

	if (msg && msg[0])
	{
		trc_println("< %s(%d) [%s] rc=%d msg=[%s]", file, line, func, rc, msg);
	}
	else
	{
		trc_println("< %s(%d) [%s] rc=%d", file, line, func, rc);
	}

	if (g_indent == 0)
	{
		sfq_trc_close();
	}
}

#endif

