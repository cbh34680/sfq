#include "sfq-lib.h"

static void split_test()
{
	const char* orgstr = "a:b::c";
	char* copy = NULL;
	int path_num = 0;
	char** strarr = NULL;

	printf("org=[%s]\n", orgstr);
	printf("count(':')=[%d]\n", sfq_count_char(':', orgstr));

	puts("===");

	copy = sfq_stradup(orgstr);
	if (! copy)
	{
		puts("copy is null");
		return;
	}

	strarr = sfq_alloc_split(':', copy, &path_num);

	if (strarr)
	{
		char** pos = NULL;

		printf("num=[%d]\n", path_num);

		pos = strarr;
		while (*pos)
		{
			printf("split=[%s]\n", *pos);
			pos++;
		}

		sfq_free_strarr(strarr);
	}
}

int main(int argc, char** argv)
{
	split_test();

	return 0;
}

