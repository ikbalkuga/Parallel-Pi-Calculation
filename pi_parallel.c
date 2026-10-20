/*
 * Parallel Pi Number Calculation - POSIX Threads Version
 *
 * Formula: Leibniz series
 * pi = 4 * sum(i = 0 to n - 1) [(-1)^i / (2*i + 1)]
 *
 * Usage:
 * ./pi_parallel <iterations> <threads>
 *
 * Example:
 * ./pi_parallel "1.000.000.000" 4
 */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PROGRAM_NAME "pi_parallel"
#define MAX_THREADS 1024U

typedef struct {
  uint64_t start;
  uint64_t end;
  long double partial_sum;
  int worker_error;
} ThreadTask;

static void print_usage(const char *program_path) {
  const char *name = (program_path != NULL && program_path[0] != '\0')
                         ? program_path
                         : PROGRAM_NAME;

  fprintf(stderr, "Usage: %s <iterations> <threads>\n", name);
  fprintf(stderr, "Example: %s 1.000.000.000 4\n", name);
}

static int parse_positive_uint64(const char *text, const char *argument_name,
                                 uint64_t *value) {
  if (text == NULL || argument_name == NULL || value == NULL) {
    fprintf(stderr, "Error: internal parser received a NULL pointer.\n");
    return 0;
  }

  char cleaned_text[128];
  int j = 0;

  for (const char *p = text; *p != '\0'; ++p) {
    if (*p == '.' || *p == ',') {
      continue;
    }
    if (j < 127) {
      cleaned_text[j++] = *p;
    } else {
      fprintf(stderr, "Error: %s input is too long.\n", argument_name);
      return 0;
    }
  }
  cleaned_text[j] = '\0';

  if (cleaned_text[0] == '\0') {
    fprintf(stderr, "Error: %s cannot be empty or contain only separators.\n",
            argument_name);
    return 0;
  }

  if (isspace((unsigned char)cleaned_text[0])) {
    fprintf(stderr, "Error: %s must not start with whitespace.\n",
            argument_name);
    return 0;
  }

  for (int i = 0; cleaned_text[i] != '\0'; ++i) {
    if (!isdigit((unsigned char)cleaned_text[i])) {
      fprintf(stderr, "Error: %s contains an invalid character: '%c'.\n",
              argument_name, cleaned_text[i]);
      return 0;
    }
  }

  char *endptr = NULL;
  errno = 0;
  unsigned long long parsed = strtoull(cleaned_text, &endptr, 10);

  if (errno == ERANGE) {
    fprintf(stderr, "Error: %s is too large for uint64_t.\n", argument_name);
    return 0;
  }

  if (errno != 0) {
    fprintf(stderr, "Error: failed to parse %s.\n", argument_name);
    return 0;
  }

  if (endptr == cleaned_text || *endptr != '\0') {
    fprintf(stderr, "Error: %s must contain digits only.\n", argument_name);
    return 0;
  }

  if (parsed == 0ULL) {
    fprintf(stderr, "Error: %s must be greater than zero.\n", argument_name);
    return 0;
  }

  *value = (uint64_t)parsed;
  return 1;
}

static void *calculate_partial_sum(void *arg) {
  if (arg == NULL) {
    return NULL;
  }

  ThreadTask *task = (ThreadTask *)arg;
  long double local_sum = 0.0L;

  if (task->end < task->start) {
    task->worker_error = 1;
    return NULL;
  }

  for (uint64_t i = task->start; i < task->end; ++i) {

    long double denominator = (2.0L * (long double)i) + 1.0L;
    long double term = 1.0L / denominator;

    if ((i % 2U) == 0U) {
      local_sum += term;
    } else {
      local_sum -= term;
    }
  }

  task->partial_sum = local_sum;
  task->worker_error = 0;
  return NULL;
}

static int allocate_thread_data(uint64_t thread_count, pthread_t **threads,
                                ThreadTask **tasks) {
  if (threads == NULL || tasks == NULL) {
    fprintf(stderr, "Error: internal allocation received a NULL pointer.\n");
    return 0;
  }

  *threads = NULL;
  *tasks = NULL;

  if (thread_count == 0 || thread_count > MAX_THREADS) {
    fprintf(stderr, "Error: thread count must be between 1 and %u.\n",
            MAX_THREADS);
    return 0;
  }

  if (thread_count > (uint64_t)(SIZE_MAX / sizeof(**threads)) ||
      thread_count > (uint64_t)(SIZE_MAX / sizeof(**tasks))) {
    fprintf(stderr, "Error: requested thread data is too large to allocate.\n");
    return 0;
  }

  *threads = calloc((size_t)thread_count, sizeof(**threads));
  *tasks = calloc((size_t)thread_count, sizeof(**tasks));

  if (*threads == NULL || *tasks == NULL) {
    fprintf(stderr,
            "Error: memory allocation failed for %" PRIu64 " thread(s).\n",
            thread_count);
    free(*threads);
    free(*tasks);
    *threads = NULL;
    *tasks = NULL;
    return 0;
  }

  return 1;
}

static int calculate_pi_parallel(uint64_t iterations, uint64_t thread_count,
                                 long double *pi_result) {
  pthread_t *threads = NULL;
  ThreadTask *tasks = NULL;
  uint64_t created_threads = 0;
  int success = 1;

  if (pi_result == NULL) {
    fprintf(stderr, "Error: result pointer is NULL.\n");
    return 0;
  }

  if (iterations == 0 || thread_count == 0) {
    fprintf(stderr,
            "Error: iterations and threads must be greater than zero.\n");
    return 0;
  }

  if (thread_count > iterations) {
    fprintf(stderr,
            "Warning: requested threads (%" PRIu64
            ") exceed iterations (%" PRIu64 "). "
            "Using %" PRIu64 " thread(s).\n",
            thread_count, iterations, iterations);
    thread_count = iterations;
  }

  if (!allocate_thread_data(thread_count, &threads, &tasks)) {
    return 0;
  }

  uint64_t base_work = iterations / thread_count;
  uint64_t extra_work = iterations % thread_count;
  uint64_t next_start = 0;

  for (uint64_t t = 0; t < thread_count; ++t) {

    uint64_t current_work = base_work + (t < extra_work ? 1U : 0U);

    if (UINT64_MAX - next_start < current_work) {
      fprintf(stderr,
              "Error: work range overflow while assigning thread tasks.\n");
      success = 0;
      break;
    }

    tasks[t].start = next_start;
    tasks[t].end = next_start + current_work;
    tasks[t].partial_sum = 0.0L;
    tasks[t].worker_error = 1;
    next_start = tasks[t].end;

    int create_result =
        pthread_create(&threads[t], NULL, calculate_partial_sum, &tasks[t]);
    if (create_result != 0) {
      fprintf(stderr, "Error: could not create thread %" PRIu64 ": %s.\n", t,
              strerror(create_result));
      success = 0;
      break;
    }

    ++created_threads;
  }

  long double total_sum = 0.0L;

  for (uint64_t t = 0; t < created_threads; ++t) {

    int join_result = pthread_join(threads[t], NULL);
    if (join_result != 0) {
      fprintf(stderr, "Error: could not join thread %" PRIu64 ": %s.\n", t,
              strerror(join_result));
      success = 0;
      continue;
    }

    if (tasks[t].worker_error != 0) {
      fprintf(stderr, "Error: worker thread %" PRIu64 " reported an error.\n",
              t);
      success = 0;
      continue;
    }

    total_sum += tasks[t].partial_sum;
  }

  if (success && next_start != iterations) {
    fprintf(stderr,
            "Error: internal work split mismatch. Assigned %" PRIu64
            " iterations but expected %" PRIu64 ".\n",
            next_start, iterations);
    success = 0;
  }

  if (success) {
    *pi_result = 4.0L * total_sum;
  }

  free(threads);
  free(tasks);
  return success;
}

static double elapsed_cpu_seconds(clock_t start, clock_t end) {
  if (start == (clock_t)-1 || end == (clock_t)-1 || end < start) {
    return -1.0;
  }
  return (double)(end - start) / (double)CLOCKS_PER_SEC;
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    fprintf(stderr, "Error: invalid number of arguments.\n");
    print_usage(argv != NULL ? argv[0] : PROGRAM_NAME);
    return EXIT_FAILURE;
  }

  uint64_t iterations = 0;
  uint64_t thread_count = 0;

  if (!parse_positive_uint64(argv[1], "iterations", &iterations)) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (!parse_positive_uint64(argv[2], "threads", &thread_count)) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (thread_count > MAX_THREADS) {
    fprintf(stderr, "Error: threads must not be greater than %u.\n",
            MAX_THREADS);
    return EXIT_FAILURE;
  }

  clock_t start_time = clock();
  if (start_time == (clock_t)-1) {
    fprintf(
        stderr,
        "Warning: could not start CPU timing. Pi will still be calculated.\n");
  }

  long double pi = 0.0L;
  if (!calculate_pi_parallel(iterations, thread_count, &pi)) {
    fprintf(stderr, "Error: parallel calculation failed.\n");
    return EXIT_FAILURE;
  }

  clock_t end_time = clock();
  if (end_time == (clock_t)-1) {
    fprintf(stderr, "Warning: could not finish CPU timing.\n");
  }

  printf("mode: parallel\n");
  printf("iterations: %" PRIu64 "\n", iterations);
  printf("threads: %" PRIu64 "\n", thread_count);
  printf("pi: %.18Lf\n", pi);

  double seconds = elapsed_cpu_seconds(start_time, end_time);
  if (seconds >= 0.0) {
    printf("cpu_time_seconds: %.6f\n", seconds);
  }

  return EXIT_SUCCESS;
}