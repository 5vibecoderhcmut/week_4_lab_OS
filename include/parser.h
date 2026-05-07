#ifndef PARSER_H
#define PARSER_H

#include "job.h"

int load_jobs_from_csv(const char *path, job_t **out_jobs, int *out_count);
void free_jobs(job_t *jobs);

#endif
