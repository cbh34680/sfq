#include "sfq-lib.h"

/*
 * global variables
 */
static struct sem_name_obj_set* GLOBAL_snos_arr = NULL;
static size_t GLOBAL_snos_arr_num = 0;

/*
void __attribute__((constructor)) localvars_create_(void);

void localvars_create_(void)
{
}
*/

void __attribute__((destructor))  localvars_destroy_(void);

/* */
struct sem_name_obj_set
{
	sfq_bool enable;

	const char* semname;
	sem_t* semobj;

	int refcount;
};

static sfq_bool is_registered_(const char* semname)
{
	sfq_bool ret = SFQ_false;
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
				ret = SFQ_true;
				break;
			}
		}
	}

	return ret;
}

static sfq_bool register_(const char* semname, sem_t* semobj)
{
	size_t realloc_size = 0;
	size_t snos_size = 0;

	struct sem_name_obj_set* snos = NULL;
	char* semname_dup = NULL;

SFQ_LIB_ENTER
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

	snos->enable = SFQ_true;
	snos->semname= semname_dup;
	snos->semobj = semobj;

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	if (SFQ_LIB_IS_FAIL())
	{
		free(semname_dup);
		semname_dup = NULL;
	}

	return SFQ_LIB_IS_SUCCESS();
}

static sfq_bool unregister_(const char* semname)
{
	sfq_bool ret = SFQ_false;
	int i = 0;
	int loop_num = 0;

	loop_num = GLOBAL_snos_arr_num;

	for (i=0; i<loop_num; i++)
	{
		sfq_bool do_free = SFQ_false;

		struct sem_name_obj_set* snos = &GLOBAL_snos_arr[i];

		if (snos->enable)
		{
			if (semname)
			{
				if (strcmp(semname, snos->semname) == 0)
				{
					do_free = SFQ_true;
				}
			}
			else
			{
				do_free = SFQ_true;
			}
		}

		if (do_free)
		{
			assert(snos->enable);
			assert(snos->semname);
			assert(snos->semobj);

			free((char*)snos->semname);
			snos->semname = NULL;

			sem_post(snos->semobj);
			sem_close(snos->semobj);
			snos->semobj = NULL;

			snos->enable = SFQ_false;

			GLOBAL_snos_arr_num--;

			ret = SFQ_true;

			if (semname)
			{
				break;
			}
		}
	}

	if (GLOBAL_snos_arr_num == 0)
	{
		free(GLOBAL_snos_arr);
		GLOBAL_snos_arr = NULL;
	}

	return ret;
}

/* -----------------------------------------------------------------------------
 *
ここから上の関数はグローバル変数を操作するため、利用前後で GLOBAL_snos_arr_mutex
をロックする
*/
static pthread_mutex_t GLOBAL_snos_arr_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * DLL アンロード時の処理
 */
void localvars_destroy_(void)
{
	sfq_unlock_semaphore(NULL);

	pthread_mutex_destroy(&GLOBAL_snos_arr_mutex);
}

/*
 *セマフォのアンロック
 */
void sfq_unlock_semaphore(const char* semname)
{
/* lock mutex */
	pthread_mutex_lock(&GLOBAL_snos_arr_mutex);

	unregister_(semname);

/* unlock mutex */
	pthread_mutex_unlock(&GLOBAL_snos_arr_mutex);
}

/*
 * セマフォのロック
 *
 * --> ロックが獲得できたときのみ global に保存する
 */

//
// セマフォ待ち時間を設ける場合は 1 に変更
//
#define SEM_TIMEDWAIT		(0)

sfq_bool sfq_lock_semaphore(const char* semname)
{
	int irc = -1;
	sfq_bool locked = SFQ_false;
	sfq_bool registered = SFQ_false;

#if SEM_TIMEDWAIT
	struct timespec tspec;
#endif
	sem_t* semobj = NULL;

SFQ_LIB_ENTER
	assert(semname);

/* lock mutex */
	pthread_mutex_lock(&GLOBAL_snos_arr_mutex);

	if (is_registered_(semname))
	{
		SFQ_FAIL(EA_SEMEXISTS, "semaphore(name=%s) is already registered", semname);
	}

#if SEM_TIMEDWAIT
	irc = clock_gettime(CLOCK_REALTIME, &tspec);
	if (irc == -1)
	{
		SFQ_FAIL(ES_CLOCKGET, "clock_gettime");
	}
	tspec.tv_sec += 5;
#endif

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

#if SEM_TIMEDWAIT
	irc = sem_timedwait(semobj, &tspec);
#else
	irc = sem_wait(semobj);
#endif

	if (irc == -1)
	{
		SFQ_FAIL(ES_SEMIO,
			"semaphore lock wait timeout, unlock command=[sfqc-sets semunlock on]");
	}
	locked = SFQ_true;

	registered = register_(semname, semobj);
	if (! registered)
	{
		SFQ_FAIL(EA_REGSEMAPHORE, "register semaphore fault(name=%s)", semname);
	}

SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		if (semobj)
		{
			if (registered)
			{
				unregister_(semname);
			}
			else
			{
				if (locked)
				{
					sem_post(semobj);
				}

				sem_close(semobj);
				semobj = NULL;
			}
		}
	}

SFQ_LIB_LEAVE

/* unlock mutex */
	pthread_mutex_unlock(&GLOBAL_snos_arr_mutex);

	return SFQ_LIB_IS_SUCCESS();
}

