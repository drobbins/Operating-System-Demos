#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define CREATION_THREAD_COUNT 5
#define COUNTER_THREAD_COUNT 5
#define SCHED_THREAD_COUNT 3
#define COUNTER_ITERATIONS 200000L
#define BUSY_WORK_ITERATIONS 20000000UL

typedef enum {
    TASK_SUM,
    TASK_VOWELS,
    TASK_REVERSE,
    TASK_FACTORIAL,
    TASK_MAX
} TaskKind;

typedef struct {
    int id;
    const char *name;
    TaskKind kind;
} DistinctTaskArgs;

typedef struct {
    int id;
    int use_mutex;
    long iterations;
} CounterTaskArgs;

typedef struct {
    const char *label;
    int requested_policy;
    int requested_priority;
    int should_attempt_change;
    int change_result;
    int change_errno;
    int effective_policy;
    int effective_priority;
    struct timespec finished_at;
} SchedulingTaskArgs;

static volatile long long shared_counter = 0;
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

static void sleep_for_ms(long milliseconds) {
    struct timespec duration;

    duration.tv_sec = milliseconds / 1000;
    duration.tv_nsec = (milliseconds % 1000) * 1000000L;
    nanosleep(&duration, NULL);
}

static const char *policy_name(int policy) {
    switch (policy) {
        case SCHED_FIFO:
            return "SCHED_FIFO";
        case SCHED_RR:
            return "SCHED_RR";
        case SCHED_OTHER:
            return "SCHED_OTHER";
#ifdef SCHED_BATCH
        case SCHED_BATCH:
            return "SCHED_BATCH";
#endif
#ifdef SCHED_IDLE
        case SCHED_IDLE:
            return "SCHED_IDLE";
#endif
        default:
            return "UNKNOWN";
    }
}

static void *run_distinct_task(void *context) {
    DistinctTaskArgs *task = (DistinctTaskArgs *)context;

    printf("[Creation] Thread %d (%s) started.\n", task->id, task->name);
    sleep_for_ms(25L * task->id);

    switch (task->kind) {
        case TASK_SUM: {
            int values[] = {4, 8, 15, 16, 23, 42};
            int total = 0;
            size_t i;

            for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
                total += values[i];
            }

            printf("[Creation] Thread %d summed a data set and got %d.\n",
                   task->id, total);
            break;
        }
        case TASK_VOWELS: {
            const char *text = "Operating systems use threads";
            int vowels = 0;
            size_t i;

            for (i = 0; text[i] != '\0'; ++i) {
                char c = text[i];
                if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
                    c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U') {
                    ++vowels;
                }
            }

            printf("[Creation] Thread %d counted %d vowels in \"%s\".\n",
                   task->id, vowels, text);
            break;
        }
        case TASK_REVERSE: {
            const char *source = "scheduler";
            char reversed[32];
            size_t length = strlen(source);
            size_t i;

            for (i = 0; i < length; ++i) {
                reversed[i] = source[length - 1 - i];
            }
            reversed[length] = '\0';

            printf("[Creation] Thread %d reversed \"%s\" into \"%s\".\n",
                   task->id, source, reversed);
            break;
        }
        case TASK_FACTORIAL: {
            int n = 6;
            int factorial = 1;
            int i;

            for (i = 2; i <= n; ++i) {
                factorial *= i;
            }

            printf("[Creation] Thread %d calculated %d! = %d.\n",
                   task->id, n, factorial);
            break;
        }
        case TASK_MAX: {
            int values[] = {9, 3, 27, 14, 18, 6};
            int maximum = values[0];
            size_t i;

            for (i = 1; i < sizeof(values) / sizeof(values[0]); ++i) {
                if (values[i] > maximum) {
                    maximum = values[i];
                }
            }

            printf("[Creation] Thread %d found a maximum value of %d.\n",
                   task->id, maximum);
            break;
        }
    }

    printf("[Creation] Thread %d (%s) finished.\n", task->id, task->name);
    return NULL;
}

static void run_thread_creation_demo(void) {
    pthread_t threads[CREATION_THREAD_COUNT];
    DistinctTaskArgs tasks[CREATION_THREAD_COUNT] = {
        {1, "sum worker", TASK_SUM},
        {2, "vowel worker", TASK_VOWELS},
        {3, "reverse worker", TASK_REVERSE},
        {4, "factorial worker", TASK_FACTORIAL},
        {5, "maximum worker", TASK_MAX}
    };
    size_t i;

    printf("\n=== 1. Thread Creation Demo ===\n");
    for (i = 0; i < CREATION_THREAD_COUNT; ++i) {
        pthread_create(&threads[i], NULL, run_distinct_task, &tasks[i]);
    }

    for (i = 0; i < CREATION_THREAD_COUNT; ++i) {
        pthread_join(threads[i], NULL);
    }
}

static void *run_counter_task(void *context) {
    CounterTaskArgs *task = (CounterTaskArgs *)context;
    long i;

    printf("[%s] Thread %d started %ld increments.\n",
           task->use_mutex ? "Sync" : "Unsync", task->id, task->iterations);

    for (i = 0; i < task->iterations; ++i) {
        if (task->use_mutex) {
            pthread_mutex_lock(&counter_mutex);
            ++shared_counter;
            pthread_mutex_unlock(&counter_mutex);
        } else {
            long long snapshot = shared_counter;
            if ((i % 1000) == 0) {
                sched_yield();
            }
            shared_counter = snapshot + 1;
        }
    }

    printf("[%s] Thread %d finished.\n",
           task->use_mutex ? "Sync" : "Unsync", task->id);
    return NULL;
}

static void run_counter_demo(int use_mutex) {
    pthread_t threads[COUNTER_THREAD_COUNT];
    CounterTaskArgs tasks[COUNTER_THREAD_COUNT];
    long long expected = COUNTER_THREAD_COUNT * COUNTER_ITERATIONS;
    size_t i;

    shared_counter = 0;
    printf("\n=== 2.%d %s Counter Demo ===\n",
           use_mutex ? 2 : 1,
           use_mutex ? "Synchronized" : "Unsynchronized");

    for (i = 0; i < COUNTER_THREAD_COUNT; ++i) {
        tasks[i].id = (int)i + 1;
        tasks[i].use_mutex = use_mutex;
        tasks[i].iterations = COUNTER_ITERATIONS;
        pthread_create(&threads[i], NULL, run_counter_task, &tasks[i]);
    }

    for (i = 0; i < COUNTER_THREAD_COUNT; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("[%s] Expected counter value: %lld\n",
           use_mutex ? "Sync" : "Unsync", expected);
    printf("[%s] Actual counter value:   %lld\n",
           use_mutex ? "Sync" : "Unsync", shared_counter);

    if (use_mutex) {
        printf("[Sync] A mutex protects the shared counter, so the result is controlled.\n");
    } else if (shared_counter != expected) {
        printf("[Unsync] Lost updates occurred because multiple threads changed the same data without coordination.\n");
    } else {
        printf("[Unsync] This run happened to match the expected value, but the code is still unsafe and may fail on another run.\n");
    }
}

static void *run_scheduling_task(void *context) {
    SchedulingTaskArgs *task = (SchedulingTaskArgs *)context;
    pthread_t self = pthread_self();
    struct sched_param param;
    unsigned long i;
    volatile unsigned long checksum = 0;
    int initial_policy;

    pthread_getschedparam(self, &initial_policy, &param);
    printf("[Scheduling] %s started with %s priority %d.\n",
           task->label, policy_name(initial_policy), param.sched_priority);

    task->change_result = 0;
    task->change_errno = 0;

    if (task->should_attempt_change) {
        struct sched_param requested_param;

        requested_param.sched_priority = task->requested_priority;
        task->change_result = pthread_setschedparam(
            self, task->requested_policy, &requested_param);
        if (task->change_result != 0) {
            task->change_errno = task->change_result;
            printf("[Scheduling] %s could not switch to %s priority %d (%s).\n",
                   task->label,
                   policy_name(task->requested_policy),
                   task->requested_priority,
                   strerror(task->change_errno));
        } else {
            printf("[Scheduling] %s requested %s priority %d successfully.\n",
                   task->label,
                   policy_name(task->requested_policy),
                   task->requested_priority);
        }
    }

    pthread_getschedparam(self, &task->effective_policy, &param);
    task->effective_priority = param.sched_priority;
    printf("[Scheduling] %s is now using %s priority %d.\n",
           task->label,
           policy_name(task->effective_policy),
           task->effective_priority);

    for (i = 0; i < BUSY_WORK_ITERATIONS; ++i) {
        checksum += (i ^ (unsigned long)task->effective_priority) & 1UL;
        if ((i % 5000000UL) == 0) {
            sched_yield();
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &task->finished_at);
    printf("[Scheduling] %s finished busy work with checksum %lu.\n",
           task->label, (unsigned long)checksum);
    return NULL;
}

static int compare_finish_times(const void *left, const void *right) {
    const SchedulingTaskArgs *first = *(const SchedulingTaskArgs * const *)left;
    const SchedulingTaskArgs *second = *(const SchedulingTaskArgs * const *)right;

    if (first->finished_at.tv_sec != second->finished_at.tv_sec) {
        return (first->finished_at.tv_sec < second->finished_at.tv_sec) ? -1 : 1;
    }

    if (first->finished_at.tv_nsec != second->finished_at.tv_nsec) {
        return (first->finished_at.tv_nsec < second->finished_at.tv_nsec) ? -1 : 1;
    }

    return 0;
}

static void run_scheduling_demo(void) {
    pthread_t threads[SCHED_THREAD_COUNT];
    SchedulingTaskArgs tasks[SCHED_THREAD_COUNT];
    SchedulingTaskArgs *ordered[SCHED_THREAD_COUNT];
    int rr_min = sched_get_priority_min(SCHED_RR);
    int rr_max = sched_get_priority_max(SCHED_RR);
    int rr_mid = rr_min + (rr_max - rr_min) / 2;
    size_t i;

    tasks[0].label = "default-thread";
    tasks[0].should_attempt_change = 0;
    tasks[0].requested_policy = SCHED_OTHER;
    tasks[0].requested_priority = 0;

    tasks[1].label = "attempt-high";
    tasks[1].should_attempt_change = 1;
    tasks[1].requested_policy = SCHED_RR;
    tasks[1].requested_priority = rr_mid > 0 ? rr_mid : 1;

    tasks[2].label = "attempt-low";
    tasks[2].should_attempt_change = 1;
    tasks[2].requested_policy = SCHED_RR;
    tasks[2].requested_priority = rr_min > 0 ? rr_min : 1;

    printf("\n=== 3. Scheduling / Priority Demo ===\n");
    printf("[Scheduling] POSIX threads often start under %s with priority 0 on Linux.\n",
           policy_name(SCHED_OTHER));
    printf("[Scheduling] This demo tries to request %s priorities %d and %d for two threads.\n",
           policy_name(SCHED_RR), tasks[1].requested_priority, tasks[2].requested_priority);

    for (i = 0; i < SCHED_THREAD_COUNT; ++i) {
        pthread_create(&threads[i], NULL, run_scheduling_task, &tasks[i]);
        ordered[i] = &tasks[i];
    }

    for (i = 0; i < SCHED_THREAD_COUNT; ++i) {
        pthread_join(threads[i], NULL);
    }

    qsort(ordered, SCHED_THREAD_COUNT, sizeof(ordered[0]), compare_finish_times);

    printf("[Scheduling] Completion order in this run:\n");
    for (i = 0; i < SCHED_THREAD_COUNT; ++i) {
        printf("  %zu. %s (%s priority %d)\n",
               i + 1,
               ordered[i]->label,
               policy_name(ordered[i]->effective_policy),
               ordered[i]->effective_priority);
    }

    if (tasks[1].change_result != 0 || tasks[2].change_result != 0) {
        printf("[Scheduling] On many systems, changing to realtime policies requires elevated privileges, so priority requests may fail or behave only as scheduler hints.\n");
    } else {
        printf("[Scheduling] Even when priorities are accepted, they influence scheduling but do not guarantee a fixed execution order.\n");
    }
}

int main(void) {
    printf("Simple POSIX thread demo for thread creation, synchronization, and scheduling.\n");

    run_thread_creation_demo();
    run_counter_demo(0);
    run_counter_demo(1);
    run_scheduling_demo();

    return EXIT_SUCCESS;
}
