#include "sfq-lib.h"

static void split_test()
{
	char* env_path = getenv("PATH");
	int path_num = 0;
	char** path_arr = sfq_alloc_split(':', env_path, &path_num);

	printf("PATH=[%s]\n", env_path);

	if (path_arr)
	{
		char** pos = NULL;

		printf("num=[%d]\n", path_num);

		pos = path_arr;
		while (*pos)
		{
			printf("split=[%s]\n", *pos);
			pos++;
		}

		sfq_free_split(path_arr);
	}
}

int main(int argc, char** argv)
{
	split_test();

	return 0;
}

