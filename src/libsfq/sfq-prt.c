#include "sfq-lib.h"

#ifdef SFQ_DEBUG_BUILD
	#define PRINT_SIZES		(0)
	#define PRINT_OPERATE_HIST1	(1)
	#define PRINT_OPERATE_HIST2	(0)
#else
	#define PRINT_SIZES		(0)
	#define PRINT_OPERATE_HIST1	(0)
	#define PRINT_OPERATE_HIST2	(0)
#endif

void sfq_print_procs(const struct sfq_process_info* procs, size_t procs_num)
{
	int i = 0;

	if (! procs)
	{
		return;
	}

	if (! procs_num)
	{
		return;
	}

	for (i=0; i<procs_num; i++)
	{
		fprintf(stderr, "- [process-%d]\n", i);
		fprintf(stderr, "- ppid                        = %u\n",  procs[i].ppid);
		fprintf(stderr, "- pid                         = %u\n",  procs[i].pid);
		fprintf(stderr, "- procstatus                  = %u\n",  procs[i].procstatus);
		fprintf(stderr, "- updtime                     = %zu\n", procs[i].updtime);
		fprintf(stderr, "- start_num                   = %zu\n", procs[i].start_num);
		fprintf(stderr, "- loop_num                    = %zu\n", procs[i].loop_num);

		fprintf(stderr, "- to_success                  = %zu\n", procs[i].to_success);
		fprintf(stderr, "- to_appexit_non0             = %zu\n", procs[i].to_appexit_non0);
		fprintf(stderr, "- to_cantexec                 = %zu\n", procs[i].to_cantexec);
		fprintf(stderr, "- to_fault                    = %zu\n", procs[i].to_fault);

		if ((i + 1) < procs_num)
		{
			fprintf(stderr, "-\n");
		}
	}
	fprintf(stderr, "\n");
}

void sfq_print_sizes(void)
{
#if PRINT_SIZES
	long sc_child_max = sysconf(_SC_CHILD_MAX);
	long sc_arg_max = sysconf(_SC_ARG_MAX);

	fprintf(stderr, "! [print_sizes]\n");
	fprintf(stderr, "! sizeof(bool)                = %zu\n", sizeof(bool));
	fprintf(stderr, "! sizeof(int)                 = %zu\n", sizeof(int));
	fprintf(stderr, "! sizeof(long)                = %zu\n", sizeof(long));
	fprintf(stderr, "! sizeof(void*)               = %zu\n", sizeof(void*));
	fprintf(stderr, "! sizeof(off_t)               = %zu\n", sizeof(off_t));
	fprintf(stderr, "! sizeof(size_t)              = %zu\n", sizeof(size_t));
	fprintf(stderr, "! sizeof(pid_t)               = %zu\n", sizeof(pid_t));
	fprintf(stderr, "! sizeof(time_t)              = %zu\n", sizeof(time_t));
	fprintf(stderr, "! sizeof(uintptr_t)           = %zu\n", sizeof(uintptr_t));
	fprintf(stderr, "! MAXNAMLEN                   = %d\n",  MAXNAMLEN);
	fprintf(stderr, "! PATH_MAX                    = %d\n",  PATH_MAX);
	fprintf(stderr, "! USHRT_MAX                   = %d\n",  USHRT_MAX);
	fprintf(stderr, "! UINT_MAX                    = %u\n",  UINT_MAX);
	fprintf(stderr, "! ULONG_MAX                   = %zu\n", ULONG_MAX);
	fprintf(stderr, "! BUFSIZ                      = %d\n",  BUFSIZ);
	fprintf(stderr, "! sc_child_max                = %ld\n", sc_child_max);
	fprintf(stderr, "! sc_arg_max                  = %ld\n", sc_arg_max);
	fprintf(stderr, "!\n");

	fprintf(stderr, "! sizeof(sfq_byte)            = %zu\n", sizeof(sfq_byte));
	fprintf(stderr, "! sizeof(sfq_process_info)    = %zu\n", sizeof(struct sfq_process_info));
	fprintf(stderr, "! sizeof(qf_header)           = %zu\n", sizeof(struct sfq_file_header));
	fprintf(stderr, "! sizeof(q_header)            = %zu\n", sizeof(struct sfq_q_header));
	fprintf(stderr, "! sizeof(qh_sval)             = %zu\n", sizeof(struct sfq_qh_sval));
	fprintf(stderr, "! sizeof(qh_dval)             = %zu\n", sizeof(struct sfq_qh_dval));
	fprintf(stderr, "! sizeof(e_header)            = %zu\n", sizeof(struct sfq_e_header));
	fprintf(stderr, "! sizeof(sfq_value)           = %zu\n", sizeof(struct sfq_value));
	fprintf(stderr, "\n");
#endif
}

static void sfq_print_qh_dval_(const struct sfq_qh_dval* p, char c)
{
	fprintf(stderr, "%c [print_q_header:dynv]\n", c);
	fprintf(stderr, "%c q_header.elm_last_push_pos  = %zu\n", c, p->elm_last_push_pos);
	fprintf(stderr, "%c q_header.elm_new_push_pos   = %zu\n", c, p->elm_new_push_pos);
	fprintf(stderr, "%c q_header.elm_next_shift_pos = %zu\n", c, p->elm_next_shift_pos);
	fprintf(stderr, "%c q_header.elm_num            = %zu\n", c, p->elm_num);
	fprintf(stderr, "%c q_header.elm_lastid         = %zu\n", c, p->elm_lastid);
	fprintf(stderr, "%c q_header.lastoper           = %s\n",  c, p->lastoper);
	fprintf(stderr, "%c q_header.questatus          = %u\n",  c, p->questatus);
	fprintf(stderr, "%c q_header.update_num         = %zu\n", c, p->update_num);
	fprintf(stderr, "%c q_header.updatetime         = %zu\n", c, p->updatetime);
	fprintf(stderr, "\n");
}

void sfq_print_qf_header(const struct sfq_file_header* p)
{
	fprintf(stderr, "# [print_qf_header:conf]\n");
	fprintf(stderr, "# qf_header.magicstr          = %s\n", p->magicstr);
	fprintf(stderr, "# qf_header.qfh_size          = %u\n", p->qfh_size);
	fprintf(stderr, "#\n");
	sfq_print_q_header(&p->qh);

#if PRINT_OPERATE_HIST1
	fprintf(stderr, "/ *** last1 ***\n");
	sfq_print_qh_dval_(&p->last_qhd1, '/');
#endif

#if PRINT_OPERATE_HIST2
	fprintf(stderr, "/ *** last2 ***\n");
	sfq_print_qh_dval_(&p->last_qhd2, '/');
#endif
}

void sfq_print_q_header(const struct sfq_q_header* p)
{
	fprintf(stderr, "# [print_q_header:conf]\n");
	fprintf(stderr, "# q_header.procseg_start_pos  = %zu\n", p->sval.procseg_start_pos);
	fprintf(stderr, "# q_header.elmseg_start_pos   = %zu\n", p->sval.elmseg_start_pos);
	fprintf(stderr, "# q_header.elmseg_end_pos     = %zu\n", p->sval.elmseg_end_pos);
	fprintf(stderr, "# q_header.filesize_limit     = %zu\n", p->sval.filesize_limit);
	fprintf(stderr, "# q_header.payloadsize_limit  = %zu\n", p->sval.payloadsize_limit);
	fprintf(stderr, "# q_header.max_proc_num       = %u\n",  p->sval.max_proc_num);
	fprintf(stderr, "#\n");

	sfq_print_qh_dval(&p->dval);
}

void sfq_print_qh_dval(const struct sfq_qh_dval* p)
{
	sfq_print_qh_dval_(p, '#');
}

void sfq_print_e_header(const struct sfq_e_header* p)
{
	char uuid_s[36 + 1] = "";

	uuid_unparse_upper(p->uuid, uuid_s);

	fprintf(stderr, "# [element]\n");
	fprintf(stderr, "# element.eh_size             = %u\n",  p->eh_size);
	fprintf(stderr, "# element.prev_elmpos         = %zu\n", p->prev_elmpos);
	fprintf(stderr, "# element.next_elmpos         = %zu\n", p->next_elmpos);
	fprintf(stderr, "# element.id                  = %zu\n", p->id);
	fprintf(stderr, "# element.pushtime            = %zu\n", p->pushtime);
	fprintf(stderr, "# element.uuid                = %s\n",  uuid_s);
	fprintf(stderr, "# element.payload_type        = %u\n",  p->payload_type);
	fprintf(stderr, "#\n");
	fprintf(stderr, "# element.execpath_size       = %u\n",  p->execpath_size);
	fprintf(stderr, "# element.execargs_size       = %u\n",  p->execargs_size);
	fprintf(stderr, "# element.metadata_size       = %u\n",  p->metadata_size);
	fprintf(stderr, "# element.payload_size        = %zu\n", p->payload_size);
	fprintf(stderr, "# element.elmmargin_          = %u\n",  p->elmmargin_);
	fprintf(stderr, "# element.elmsize_            = %zu\n", p->elmsize_);
	fprintf(stderr, "\n");
}

