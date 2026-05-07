#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"

int parse_policy(const char *text, policy_t *out_policy);
const char *policy_to_string(policy_t policy);
job_t *scheduler_get_next_job(scheduler_context_t *ctx);

#endif
