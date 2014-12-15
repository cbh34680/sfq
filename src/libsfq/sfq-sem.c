#include "sfq-lib.h"

struct sem_name_obj_set
{
	bool enable;

	const char* semname;
	sem_t* semobj;
};

static struct sem_name_obj_set* GLOBAL_snos_arr = NULL;
static size_t GLOBAL_snos_arr_num = 0;

static bool register_(const char* semname, sem_t* semobj)
{
	size_t realloc_size = 0;
	size_t snos_size = 0;

	struct sem_name_obj_set* snos = NULL;
	char* semname_dup = NULL;

SFQ_LIB_INITIALIZE

	snos_size = sizeof(struct sem_name_obj_set);

	assert(semname);
	assert(semname[0]);
	assert(semobj);

	semname_dup = strdup(semname);
	if (! semname_dup)
	{
		SFQ_FAIL(ES_MEMALLOC, "strdup");
	}

	realloc_size = (snos_size * (GLOBAL_snos_arr_num + 1));
	
	GLOBAL_snos_arr = realloc(GLOBAL_snos_arr, realloc_size);
	if (! GLOBAL_snos_arr)
	{
		SFQ_FAIL(ES_MEMALLOC, "realloc");
	}

	snos = &GLOBAL_snos_arr[GLOBAL_snos_arr_num];
	GLOBAL_snos_arr_num++;

	bzero(snos, snos_size);

//printf("reg %s\n", semname);
	snos->enable = true;
	snos->semname= semname_dup;
	snos->semobj = semobj;

SFQ_LIB_CHECKPOINT

SFQ_LIB_FINALIZE

	if (SFQ_LIB_IS_FAIL())
	{
		free(semname_dup);
		semname_dup = NULL;
	}

	return SFQ_LIB_IS_SUCCESS();
}

static bool is_registered_(const char* semname)
{
	bool ret = false;
	int i = 0;

	assert(semname);
	assert(semname[0]);

	for (i=0; i<GLOBAL_snos_arr_num; i++)
	{
		struct sem_name_obj_set* snos = &GLOBAL_snos_arr[i];

		if (snos->enable)
		{
			if (strcmp(semname, snos->semname) == 0)
			{
				ret = true;
				break;
			}
		}
	}

//printf("is_reg? %s .. %d\n", semname, ret);

	return ret;
}

static bool unregister_(const char* semname)
{
	bool ret = false;
	int i = 0;
	int loop_num = 0;

	loop_num = GLOBAL_snos_arr_num;

//printf("unreg %s (loop=%d)\n", semname, loop_num);

	for (i=0; i<loop_num; i++)
	{
		bool do_free = false;

		struct sem_name_obj_set* snos = &GLOBAL_snos_arr[i];

		if (snos->enable)
		{
			if (semname)
			{
				if (strcmp(semname, snos->semname) == 0)
				{
					do_free = true;
				}
			}
			else
			{
				do_free = true;
			}
		}

		if (do_free)
		{
//printf("unreg %s do free\n", semname);
			assert(snos->enable);
			assert(snos->semname);
			assert(snos->semobj);

			free((char*)snos->semname);
			snos->semname = NULL;

			sem_post(snos->semobj);
			sem_close(snos->semobj);
			snos->semobj = NULL;

			snos->enable = false;

			GLOBAL_snos_arr_num--;

			ret = true;

			if (semname)
			{
				break;
			}
		}
	}

	if (GLOBAL_snos_arr_num == 0)
	{
//printf("unreg release all\n");
		free(GLOBAL_snos_arr);
		GLOBAL_snos_arr = NULL;
	}

	return ret;
}

/*
セマフォのアンロック
*/
void sfq_unlock_semaphore(const char* semname)
{
	unregister_(semname);
}

/*
セマフォのロック

--> ロックが獲得できたときのみ global に保存する
*/
bool sfq_lock_semaphore(const char* semname)
{
	int irc = -1;
	bool locked = false;
	bool registered = false;

	struct timespec tspec;
	sem_t* semobj = NULL;

SFQ_LIB_INITIALIZE

	assert(semname);

	if (is_registered_(semname))
	{
		SFQ_FAIL(EA_SEMEXISTS, "semaphore(name=%s) is already registered", semname);
	}

	irc = clock_gettime(CLOCK_REALTIME, &tspec);
	if (irc == -1)
	{
		SFQ_FAIL(ES_CLOCKGET, "clock_gettime");
	}
	tspec.tv_sec += 2;

/* open semaphore */
/*
本来は適切なパーミッションとすべきだが、マシンを再起動すると
セマフォが消えてしまうので 0666 としておく。
*/
	semobj = sem_open(semname, O_CREAT, 0666, 1);
	if (semobj == SEM_FAILED /* =0 */)
	{
		SFQ_FAIL(ES_SEMOPEN, "semaphore open error, check permission file=[/dev/shm%s]",
			semname);
	}

//printf("lock attempt to get\n");
	//irc = sem_wait(semobj);

	irc = sem_timedwait(semobj, &tspec);
	if (irc == -1)
	{
		SFQ_FAIL(ES_SEMIO, "semaphore lock wait timeout, unlock command=[sfqc-sets semunlock on]");
	}
	locked = true;

//printf("lock get\n");
	registered = register_(semname, semobj);
	if (! registered)
	{
		SFQ_FAIL(EA_REGSEMAPHORE, "register semaphore fault(name=%s)", semname);
	}
//printf("lock reg done\n");

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		if (semobj)
		{
			if (registered)
			{
//printf("lock unreg\n");
				unregister_(semname);
			}
			else
			{
				if (locked)
				{
//printf("lock release\n");
					sem_post(semobj);
				}

//printf("lock close\n");
				sem_close(semobj);
				semobj = NULL;
			}
		}
	}

SFQ_LIB_FINALIZE

	return SFQ_LIB_IS_SUCCESS();
}

