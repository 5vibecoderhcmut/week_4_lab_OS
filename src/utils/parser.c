#include "parser.h"
#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trim_in_place(char *text) {
    char *end;

    while (isspace((unsigned char)*text)) {
        text++;
    }

    if (*text == '\0') {
        return text;
    }

    end = text + strlen(text) - 1;
    while (end > text && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    return text;
}

static int parse_int_field(const char *text, int *out) {
    char *end = NULL;
    long value;

    if (text == NULL || out == NULL || *text == '\0') {
        return -1;
    }

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value < -2147483647L || value > 2147483647L) {
        return -1;
    }

    *out = (int)value;
    return 0;
}

static int copy_field(char *dest, size_t dest_size, const char *src) {
    if (dest == NULL || src == NULL || dest_size == 0 || strlen(src) >= dest_size) {
        return -1;
    }

    memcpy(dest, src, strlen(src) + 1);
    return 0;
}

static int parse_job_line(char *line, int line_no, job_t *job) {
    char *fields[6];
    char *token;
    char *saveptr = NULL;
    int field_count = 0;

    token = strtok_r(line, ",", &saveptr);
    while (token != NULL && field_count < 6) {
        fields[field_count] = trim_in_place(token);
        field_count++;
        token = strtok_r(NULL, ",", &saveptr);
    }

    if (field_count != 6 || token != NULL) {
        fprintf(stderr, "CSV parse error at line %d: expected 6 fields\n", line_no);
        return -1;
    }

    memset(job, 0, sizeof(*job));
    if (parse_int_field(fields[0], &job->job_id) != 0 ||
        parse_int_field(fields[2], &job->arrival_time) != 0 ||
        parse_int_field(fields[3], &job->estimated_runtime) != 0 ||
        parse_int_field(fields[4], &job->priority) != 0) {
        fprintf(stderr, "CSV parse error at line %d: invalid integer field\n", line_no);
        return -1;
    }

    if (job->job_id <= 0 || job->arrival_time < 0 || job->estimated_runtime <= 0 || job->priority <= 0) {
        fprintf(stderr, "CSV parse error at line %d: invalid job values\n", line_no);
        return -1;
    }

    if (copy_field(job->seller_id, sizeof(job->seller_id), fields[1]) != 0 ||
        copy_field(job->job_type, sizeof(job->job_type), fields[5]) != 0) {
        fprintf(stderr, "CSV parse error at line %d: text field too long\n", line_no);
        return -1;
    }

    job->start_time = -1;
    job->finish_time = -1;
    job->status = JOB_NEW;
    job->assigned_worker_id = -1;
    job->sequence_no = -1;
    return 0;
}

int load_jobs_from_csv(const char *path, job_t **out_jobs, int *out_count) {
    FILE *file;
    job_t *jobs;
    int capacity = 16;
    int count = 0;
    int line_no = 0;
    char line[MAX_LINE_LEN];

    if (path == NULL || out_jobs == NULL || out_count == NULL) {
        return -1;
    }

    *out_jobs = NULL;
    *out_count = 0;

    file = fopen(path, "r");
    if (file == NULL) {
        perror(path);
        return -1;
    }

    jobs = calloc((size_t)capacity, sizeof(job_t));
    if (jobs == NULL) {
        fclose(file);
        return -1;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *trimmed;
        job_t parsed;

        line_no++;
        trimmed = trim_in_place(line);
        if (*trimmed == '\0') {
            continue;
        }

        if (line_no == 1 && strncmp(trimmed, "job_id", 6) == 0) {
            continue;
        }

        if (count >= capacity) {
            int new_capacity = capacity * 2;
            job_t *new_jobs = realloc(jobs, (size_t)new_capacity * sizeof(job_t));
            if (new_jobs == NULL) {
                free(jobs);
                fclose(file);
                return -1;
            }
            jobs = new_jobs;
            capacity = new_capacity;
        }

        if (parse_job_line(trimmed, line_no, &parsed) != 0) {
            free(jobs);
            fclose(file);
            return -1;
        }

        jobs[count] = parsed;
        count++;
    }

    if (ferror(file)) {
        perror(path);
        free(jobs);
        fclose(file);
        return -1;
    }

    fclose(file);

    if (count == 0) {
        fprintf(stderr, "CSV parse error: no jobs found in %s\n", path);
        free(jobs);
        return -1;
    }

    *out_jobs = jobs;
    *out_count = count;
    return 0;
}

void free_jobs(job_t *jobs) {
    free(jobs);
}
