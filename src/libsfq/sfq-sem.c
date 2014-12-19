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

	pid_t pid;
	pid_t tid;
};

static void unlock_semaphore_(const char* semname)
{
	size_t free_times = 0;

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
					if (snos->pid == getpid())
					{
						if (snos->tid == sfq_gettid())
						{
							do_free = SFQ_true;
						}
					}
				}
			}
			else
			{
				do_free = SFQ_true;
			}
		}

		if (do_free)
		{
			free_times++;

			free((char*)snos->semname);
			snos->semname = NULL;

			sem_post(snos->semobj);
			sem_close(snos->semobj);
			snos->semobj = SEM_FAILED;

			snos->enable = SFQ_false;

			GLOBAL_snos_arr_num--;

			if (semname)
			{
				break;
			}
		}
	}

	if (semname)
	{
		assert(free_times);
	}

	if (GLOBAL_snos_arr_num == 0)
	{
		free(GLOBAL_snos_arr);
		GLOBAL_snos_arr = NULL;
	}
}

//
// セマフォ待ち時間を設ける場合は 1 に変更
//
#define SEM_TIMEDWAIT		(0)

static sfq_bool lock_semaphore_(const char* semname)
{
	size_t realloc_size = 0;
	struct sem_name_obj_set* snos = NULL;

	sem_t* semobj = SEM_FAILED;
	char* semname_dup = NULL;

	sfq_bool locked = SFQ_false;
	sfq_bool registered = SFQ_false;

	int irc = -1;
	int i = 0;

#if SEM_TIMEDWAIT
	struct timespec tspec;
#endif

SFQ_LIB_ENTER
	assert(semname);
	assert(semname[0]);

	for (i=0; i<GLOBAL_snos_arr_num; i++)
	{
		struct sem_name_obj_set* snos = &GLOBAL_snos_arr[i];

		if (! snos->enable)
		{
			continue;
		}

		if (strcmp(semname, snos->semname) == 0)
		{
			if (snos->pid == getpid())
			{
				if (snos->tid == sfq_gettid())
				{
					abort();
					//SFQ_FAIL(EA_REGSEMAPHORE, "already locked (%s)", semname);
				}
			}
		}
	}


/*
登録がなければオブジェクトを作成する
*/
	semname_dup = strdup(semname);
	if (! semname_dup)
	{
		SFQ_FAIL(ES_MEMALLOC, "strdup");
	}

/*
本来は適切なパーミッションとすべきだが、マシンを再起動すると
セマフォが消えてしまうので 0666 としておく。
*/
	semobj = sem_open(semname, O_CREAT, 0666, 1);
	if (semobj == SEM_FAILED /* =0 */)
	{
		SFQ_FAIL(ES_SEMOPEN,
			"semaphore open error, check permission file=[/dev/shm%s]", semname);
	}

#if SEM_TIMEDWAIT
	irc = clock_gettime(CLOCK_REALTIME, &tspec);
	if (irc == -1)
	{
		SFQ_FAIL(ES_CLOCKGET, "clock_gettime");
	}
	tspec.tv_sec += 1;

	irc = sem_timedwait(semobj, &tspec);
#else
	irc = sem_wait(semobj);
#endif

	if (irc == -1)
	{
		SFQ_FAIL(ES_SEMIO,
			"semaphore lock error, try unlock command=[sfqc-sets semunlock on]");
	}

	locked = SFQ_true;

/*
sem_wait() が成功したら登録する
*/
	realloc_size = (sizeof(struct sem_name_obj_set) * (GLOBAL_snos_arr_num + 1));

	GLOBAL_snos_arr = realloc(GLOBAL_snos_arr, realloc_size);
	if (! GLOBAL_snos_arr)
	{
		SFQ_FAIL(ES_MEMALLOC, "realloc");
	}

	snos = &GLOBAL_snos_arr[GLOBAL_snos_arr_num];
	GLOBAL_snos_arr_num++;

	bzero(snos, sizeof(struct sem_name_obj_set));

	snos->enable = SFQ_true;
	snos->semname= semname_dup;
	snos->semobj = semobj;
	snos->pid = getpid();
	snos->tid = sfq_gettid();

/*
登録が成功したら semobj は無効にする
*/
	registered = SFQ_true;
	semobj = SEM_FAILED;


SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
		free(semname_dup);
		semname_dup = NULL;

		if (registered)
		{
/*
登録済であった場合
*/
			unlock_semaphore_(semname);
		}
		else
		{
			if (semobj != SEM_FAILED)
			{
/*
この関数内で semobj を生成したが未登録の場合はここで開放する
*/
				if (locked)
				{
					sem_post(semobj);
				}

				sem_close(semobj);
				semobj = SEM_FAILED;
			}
		}
	}

SFQ_LIB_LEAVE

	return SFQ_LIB_IS_SUCCESS();
}

static pthread_mutex_t GLOBAL_snos_arr_mutex = PTHREAD_MUTEX_INITIALIZER;

void sfq_entp_critical_section_enter()
{
	pthread_mutex_lock(&GLOBAL_snos_arr_mutex);
}

void sfq_entp_critical_section_leave()
{
	pthread_mutex_unlock(&GLOBAL_snos_arr_mutex);
}

/*
 * DLL アンロード時の処理
 */
void localvars_destroy_(void)
{
/*
全てのセマフォをアンロック
*/
	unlock_semaphore_(NULL);

	pthread_mutex_destroy(&GLOBAL_snos_arr_mutex);
}

/*
 *セマフォのアンロック
 */
void sfq_unlock_semaphore(const char* semname)
{
	unlock_semaphore_(semname);
}

/*
 * セマフォのロック
 */
sfq_bool sfq_lock_semaphore(const char* semname)
{
	return lock_semaphore_(semname);
}

