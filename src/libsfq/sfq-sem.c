#include "sfq-lib.h"

/*
 * global variables
 */
static struct sem_name_obj_set* GLOBAL_snos_arr = NULL;
static size_t GLOBAL_snos_arr_num = 0;
static size_t GLOBAL_snos_enable_num = 0;

static pthread_mutex_t GLOBAL_snos_arr_mutex = PTHREAD_MUTEX_INITIALIZER;

#if 0
	#define LOG_(a) \
		fprintf(stderr, "%s(%d):%s:%d:%d: [%s]\n", \
			__FILE__, __LINE__, __func__, getpid(), sfq_gettid(), (a));

	#define LOGLF_() \
		fprintf(stderr, "\n");

#else
	#define LOG_(a)
	#define LOGLF_()
#endif

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

static sfq_bool register_(const char* semname, sem_t* semobj)
{
	size_t realloc_size = 0;
	struct sem_name_obj_set* snos = NULL;
	const char* semname_dup = NULL;

LOG_("a-1");
	pthread_mutex_lock(&GLOBAL_snos_arr_mutex);
LOG_("a-2");

SFQ_LIB_ENTER

	semname_dup = strdup(semname);
	if (! semname_dup)
	{
		SFQ_FAIL(ES_MEMALLOC, "strdup");
	}

/*
配列を広げる
*/
	realloc_size = (sizeof(struct sem_name_obj_set) * (GLOBAL_snos_arr_num + 1));

	GLOBAL_snos_arr = realloc(GLOBAL_snos_arr, realloc_size);
	if (! GLOBAL_snos_arr)
	{
		SFQ_FAIL(ES_MEMALLOC, "realloc");
	}

	snos = &GLOBAL_snos_arr[GLOBAL_snos_arr_num];

	GLOBAL_snos_arr_num++;
	GLOBAL_snos_enable_num++;

/*
要素を書き込む = 登録
*/
	bzero(snos, sizeof(struct sem_name_obj_set));

	snos->enable = SFQ_true;
	snos->semname= semname_dup;
	snos->semobj = semobj;
	snos->pid = getpid();
	snos->tid = sfq_gettid();
LOG_("a-3");

SFQ_LIB_CHECKPOINT

SFQ_LIB_LEAVE

	pthread_mutex_unlock(&GLOBAL_snos_arr_mutex);
LOG_("a-4");

	return SFQ_LIB_IS_SUCCESS();
}

static void unregister_all_()
{
	pthread_mutex_destroy(&GLOBAL_snos_arr_mutex);

	int i = 0;
	int loop_num = 0;

LOG_("e-1");
	pthread_mutex_lock(&GLOBAL_snos_arr_mutex);
LOG_("e-2");

	if (GLOBAL_snos_arr_num)
	{
		loop_num = GLOBAL_snos_arr_num;

		for (i=0; i<loop_num; i++)
		{
			struct sem_name_obj_set* snos = &GLOBAL_snos_arr[i];

			if (snos->enable)
			{
				snos->enable = SFQ_false;

				free((char*)snos->semname);
				snos->semname = NULL;

				sem_post(snos->semobj);
				sem_close(snos->semobj);
				snos->semobj = SEM_FAILED;

				GLOBAL_snos_enable_num--;

				break;
			}
		}

LOG_("e-4");
		free(GLOBAL_snos_arr);
		GLOBAL_snos_arr = NULL;

		GLOBAL_snos_arr_num = 0;
LOG_("e-5");
	}
LOG_("e-6");
}

static sem_t* unregister_(const char* semname)
{
	sem_t* semobj = SEM_FAILED;

	int i = 0;
	int loop_num = 0;

LOG_("b-1");
	pthread_mutex_lock(&GLOBAL_snos_arr_mutex);
LOG_("b-2");

	loop_num = GLOBAL_snos_arr_num;

	for (i=0; i<loop_num; i++)
	{
		struct sem_name_obj_set* snos = &GLOBAL_snos_arr[i];

		if (snos->enable)
		{
			if (strcmp(semname, snos->semname) == 0)
			{
				if (snos->pid == getpid())
				{
					if (snos->tid == sfq_gettid())
					{
						semobj = snos->semobj;

/*
配列中の要素を無効にする

--> semobj については返却先で post(), close() する
*/
						snos->enable = SFQ_false;

						free((char*)snos->semname);
						snos->semname = NULL;

						snos->semobj = SEM_FAILED;

						GLOBAL_snos_enable_num--;
LOG_("b-3");

						break;
					}
				}
			}
		}
	}

LOG_("b-4");
	if (GLOBAL_snos_enable_num == 0)
	{
/*
有効な要素がなくなったら配列を開放する
*/
		free(GLOBAL_snos_arr);
		GLOBAL_snos_arr = NULL;

		GLOBAL_snos_arr_num = 0;
LOG_("b-5");
	}

LOG_("b-6");
	pthread_mutex_unlock(&GLOBAL_snos_arr_mutex);
LOG_("b-7");

	return semobj;
}

/*
 * DLL アンロード時の処理
 */
void localvars_destroy_(void)
{
/*
全てのセマフォをアンロック
*/
	unregister_all_();

}

static sfq_bool lock_semaphore_(const char* semname, int semlock_wait_sec)
{
	sem_t* semobj = SEM_FAILED;
	char* semname_dup = NULL;

	sfq_bool locked = SFQ_false;
	sfq_bool registered = SFQ_false;

	int irc = -1;

	struct timespec tspec;

SFQ_LIB_ENTER
	assert(semname);
	assert(semname[0]);
LOG_("c-1");

/*
登録がなければオブジェクトを作成する

--> 本来は適切なパーミッションとすべきだが、マシンを再起動すると消えてしまうので 0666 としておく
*/
	semobj = sem_open(semname, O_CREAT, 0666, 1);
	if (semobj == SEM_FAILED)
	{
		SFQ_FAIL(ES_SEMOPEN,
			"semaphore open error, check permission file=[/dev/shm%s]", semname);
	}

/*
セマフォのロック
*/
LOG_("c-2");
	if (semlock_wait_sec > 0)
	{
		irc = clock_gettime(CLOCK_REALTIME, &tspec);
		if (irc == -1)
		{
			SFQ_FAIL(ES_CLOCKGET, "clock_gettime");
		}
		tspec.tv_sec += semlock_wait_sec;

		irc = sem_timedwait(semobj, &tspec);
	}
	else
	{
		irc = sem_wait(semobj);
	}
LOG_("c-3");

	if (irc == -1)
	{
		SFQ_FAIL(ES_SEMIO,
			"semaphore lock error, try unlock command=[sfqc-sets {-N quename} semunlock on]");
	}

	locked = SFQ_true;

/*
sem_wait() が成功したら登録する
*/
	registered = register_(semname, semobj);
LOG_("c-4");


SFQ_LIB_CHECKPOINT

	if (SFQ_LIB_IS_FAIL())
	{
LOG_("c-5");
		free(semname_dup);
		semname_dup = NULL;

		if (registered)
		{
/*
登録済であった場合
*/
			unregister_(semname);
		}

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
LOG_("c-6");

SFQ_LIB_LEAVE

	return SFQ_LIB_IS_SUCCESS();
}

static void unlock_semaphore_(const char* semname)
{
	sem_t* semobj = SEM_FAILED;

LOG_("d-1");
	if (semname)
	{
LOG_("d-2");
		semobj = unregister_(semname);
LOG_("d-3");

		if (semobj != SEM_FAILED)
		{
LOG_("d-4");
			sem_post(semobj);
			sem_close(semobj);

			semobj = SEM_FAILED;
		}
LOG_("d-5");
	}
	else
	{
LOG_("d-6");
		unregister_all_();
LOG_("d-7");
	}
LOG_("d-8");
}


/*
 *セマフォのアンロック
 */
void sfq_unlock_semaphore(const char* semname)
{
LOG_("B-1");
	unlock_semaphore_(semname);
LOG_("B-2");
LOGLF_();
}

/*
 * セマフォのロック
 */
sfq_bool sfq_lock_semaphore(const char* semname, int semlock_wait_sec)
{
	sfq_bool b = SFQ_false;
LOG_("A-1");
	b = lock_semaphore_(semname, semlock_wait_sec);
LOG_("A-2");
LOGLF_();

	return b;
}

