#include <stdio.h>
#include <stdlib.h>

#include "scheduler.h"

/*
    ============================================================
    MAIN PROGRAM
    ============================================================

    Cách chạy:
        ./scheduler <csv_file> <policy> <worker_count>

    Ví dụ:
        ./scheduler workloads/workload_a.csv fifo 4
        ./scheduler workloads/workload_b.csv sjf 4
        ./scheduler workloads/workload_c.csv priority 4
*/

static void print_usage(const char* program_name) {
    printf("Usage:\n");
    printf("  %s <csv_file> <policy> <worker_count>\n", program_name);
    printf("\n");
    printf("Policies:\n");
    printf("  fifo\n");
    printf("  sjf\n");
    printf("  priority\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s workloads/workload_a.csv fifo 4\n", program_name);
    printf("  %s workloads/workload_b.csv sjf 4\n", program_name);
    printf("  %s workloads/workload_c.csv priority 4\n", program_name);
}

int main(int argc, char* argv[]) {
    const char* csv_file;
    const char* policy_str;
    int input_worker_count;

    /*
        Chương trình cần đúng 3 tham số:
        argv[1] = đường dẫn file CSV
        argv[2] = policy: fifo / sjf / priority
        argv[3] = số worker
    */
    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }

    csv_file = argv[1];
    policy_str = argv[2];
    input_worker_count = atoi(argv[3]);

    if (input_worker_count <= 0) {
        printf("Error: worker_count must be a positive integer.\n");
        return 1;
    }

    /*
        Parse policy từ string sang enum.
        Ví dụ:
            "fifo" -> POLICY_FIFO
    */
    if (!parse_policy(policy_str, &current_policy)) {
        printf("Error: invalid policy '%s'.\n", policy_str);
        print_usage(argv[0]);
        return 1;
    }

    /*
        Đọc file CSV vào mảng jobs.
        Sau hàm này:
            jobs != NULL
            total_jobs > 0
    */
    if (!load_jobs_from_csv(csv_file)) {
        printf("Error: cannot load jobs from file '%s'.\n", csv_file);
        return 1;
    }

    printf("Loaded %d jobs from %s\n", total_jobs, csv_file);
    printf("Policy: %s\n", policy_to_string(current_policy));
    printf("Workers: %d\n", input_worker_count);
    printf("========================================\n");

    /*
        Chạy hệ thống worker pool.
        Đây là phần Student 2.
    */
    run_worker_system(input_worker_count);

    printf("========================================\n");

    /*
        In metrics cuối chương trình.
    */
    print_summary(current_policy, input_worker_count);

    /*
        Giải phóng mảng jobs.
    */
    free_jobs();

    return 0;
}