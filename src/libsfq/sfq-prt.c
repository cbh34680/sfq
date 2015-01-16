#include "sfq-lib.h"

#ifdef SFQ_DEBUG_BUILD
	#define PRINT_OPERATE_HIST1	(1)
	#define PRINT_OPERATE_HIST2	(1)
#else
	#define PRINT_OPERATE_HIST1	(1)
	#define PRINT_OPERATE_HIST2	(0)
#endif

void sfq_print_qo(const struct sfq_queue_object* qo)
{
	fprintf(stderr, "! [queue]\n");
	fprintf(stderr, "! om.querootdir:    %s\n",  qo->om->querootdir);
	fprintf(stderr, "! om.quename:       %s\n",  qo->om->quename);
	fprintf(stderr, "! om.quedir:        %s\n",  qo->om->quedir);
	fprintf(stderr, "! om.quefile:       %s\n",  qo->om->quefile);
	fprintf(stderr, "! om.quelogdir:     %s\n",  qo->om->quelogdir);
	fprintf(stderr, "! om.queproclogdir: %s\n",  qo->om->queproclogdir);
	fprintf(stderr, "! om.queexeclogdir: %s\n",  qo->om->queexeclogdir);
	fprintf(stderr, "! om.semname:       %s\n",  qo->om->semname);
	fprintf(stderr, "! om.opentime:      %zu\n", qo->opentime);
	fprintf(stderr, "\n");
}

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
		const struct sfq_process_info* proc = &procs[i];

		if (proc->procstate == SFQ_PIS_LOCK)
		{
			continue;
		}

		fprintf(stderr, "- [process-%d]\n", i);
		fprintf(stderr, "- ppid                        = %u\n",  proc->ppid);
		fprintf(stderr, "- pid                         = %u\n",  proc->pid);
		fprintf(stderr, "- procstate                   = %u\n",  proc->procstate);
		fprintf(stderr, "- updtime                     = %zu\n", proc->updtime);
		fprintf(stderr, "- start_cnt                   = %zu\n", proc->start_cnt);
		fprintf(stderr, "- loop_cnt                    = %zu\n", proc->loop_cnt);

		fprintf(stderr, "- tos_success                 = %zu\n", proc->tos_success);
		fprintf(stderr, "- tos_appexit_non0            = %zu\n", proc->tos_appexit_non0);
		fprintf(stderr, "- tos_cantexec                = %zu\n", proc->tos_cantexec);
		fprintf(stderr, "- tos_fault                   = %zu\n", proc->tos_fault);

		if ((i + 1) < procs_num)
		{
			fprintf(stderr, "-\n");
		}
		else
		{
			fprintf(stderr, "\n");
		}
	}
}

void sfq_print_sizes(void)
{
	long sc_child_max = sysconf(_SC_CHILD_MAX);
	long sc_arg_max = sysconf(_SC_ARG_MAX);

	fprintf(stderr, "! [type]\n");
	fprintf(stderr, "! sizeof(sfq_bool)            = %zu\n", sizeof(sfq_bool));
	fprintf(stderr, "! sizeof(sfq_byte)            = %zu\n", sizeof(sfq_byte));
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

	fprintf(stderr, "! [struct]\n");
	fprintf(stderr, "! sizeof(process_info)        = %zu\n", sizeof(struct sfq_process_info));
	fprintf(stderr, "! sizeof(qh_sval)             = %zu\n", sizeof(struct sfq_qh_sval));
	fprintf(stderr, "! sizeof(qh_dval)             = %zu\n", sizeof(struct sfq_qh_dval));
	fprintf(stderr, "! sizeof(q_header)            = %zu\n", sizeof(struct sfq_q_header));
	fprintf(stderr, "! sizeof(file_stamp)          = %zu\n", sizeof(struct sfq_file_stamp));
	fprintf(stderr, "! sizeof(file_header)         = %zu\n", sizeof(struct sfq_file_header));
	fprintf(stderr, "! sizeof(e_header)            = %zu\n", sizeof(struct sfq_e_header));
	fprintf(stderr, "\n");
}

static void sfq_print_qh_dval_(const struct sfq_qh_dval* p, char c)
{
	fprintf(stderr, "%c [print_q_header:dynv]\n", c);
	fprintf(stderr, "%c q_header.elm_next_pop_pos   = %zu\n", c, p->elm_next_pop_pos);
	fprintf(stderr, "%c q_header.elm_next_push_pos  = %zu\n", c, p->elm_next_push_pos);
	fprintf(stderr, "%c q_header.elm_next_shift_pos = %zu\n", c, p->elm_next_shift_pos);
	fprintf(stderr, "%c q_header.elm_num            = %zu\n", c, p->elm_num);
	fprintf(stderr, "%c q_header.elm_lastid         = %zu\n", c, p->elm_lastid);
	fprintf(stderr, "%c q_header.questate           = %u\n",  c, p->questate);
	fprintf(stderr, "%c q_header.update_cnt         = %zu\n", c, p->update_cnt);
	fprintf(stderr, "%c q_header.updatetime         = %zu\n", c, p->updatetime);
	fprintf(stderr, "%c q_header.lastoper           = %s\n",  c, p->lastoper);
	fprintf(stderr, "\n");
}

void sfq_print_qf_header(const struct sfq_file_header* p)
{
	fprintf(stderr, "# [print_qf_header:conf]\n");
	fprintf(stderr, "# qf_header.qfs.magicstr      = %s\n", p->qfs.magicstr);
	fprintf(stderr, "# qf_header.qfs.qfh_size      = %u\n", p->qfs.qfh_size);
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
	fprintf(stderr, "# q_header.procs_num          = %u\n",  p->sval.procs_num);
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

	uuid_unparse(p->uuid, uuid_s);

	fprintf(stderr, "# [element]\n");
	fprintf(stderr, "# element.eh_size             = %u\n",  p->eh_size);
	fprintf(stderr, "# element.prev_elmpos         = %zu\n", p->prev_elmpos);
	fprintf(stderr, "# element.next_elmpos         = %zu\n", p->next_elmpos);
	fprintf(stderr, "# element.id                  = %zu\n", p->id);
	fprintf(stderr, "# element.pushtime            = %zu\n", p->pushtime);
	fprintf(stderr, "# element.uuid                = %s\n",  uuid_s);
	fprintf(stderr, "# element.payload_type        = %u\n",  p->payload_type);
	fprintf(stderr, "# element.execpath_size       = %u\n",  p->execpath_size);
	fprintf(stderr, "# element.execargs_size       = %u\n",  p->execargs_size);
	fprintf(stderr, "# element.metatext_size       = %u\n",  p->metatext_size);
	fprintf(stderr, "# element.payload_size        = %zu\n", p->payload_size);
	fprintf(stderr, "# element.elmmargin_          = %u\n",  p->elmmargin_);
	fprintf(stderr, "# element.elmsize_            = %zu\n", p->elmsize_);
	fprintf(stderr, "\n");
}

