/*
 * Parallel Pi Number Calculation - Serial Version (Robust Digit Separator
 * Support)
 *
 * Formula: Leibniz series
 * pi = 4 * sum(i = 0 to n - 1) [(-1)^i / (2*i + 1)]
 *
 * Usage:
 * ./pi_serial <iterations>
 *
 * Example:
 * ./pi_serial "1.000.000.000"
 */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PROGRAM_NAME "pi_serial"

static void print_usage(const char *program_path) {
  const char *name = (program_path != NULL && program_path[0] != '\0')
                         ? program_path
                         : PROGRAM_NAME;

  fprintf(stderr, "Usage: %s <iterations>\n", name);
  fprintf(stderr, "Example: %s 1.000.000.000\n", name);
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

static long double calculate_pi_serial(uint64_t iterations) {
  long double sum = 0.0L;

  for (uint64_t i = 0; i < iterations; ++i) {

    long double denominator = (2.0L * (long double)i) + 1.0L;
    long double term = 1.0L / denominator;

    if ((i % 2U) == 0U) {
      sum += term;
    } else {
      sum -= term;
    }
  }

  return 4.0L * sum;
}

static double elapsed_cpu_seconds(clock_t start, clock_t end) {
  if (start == (clock_t)-1 || end == (clock_t)-1 || end < start) {
    return -1.0;
  }

  return (double)(end - start) / (double)CLOCKS_PER_SEC;
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    fprintf(stderr, "Error: invalid number of arguments.\n");
    print_usage(argv != NULL ? argv[0] : PROGRAM_NAME);
    return EXIT_FAILURE;
  }

  uint64_t iterations = 0;

  if (!parse_positive_uint64(argv[1], "iterations", &iterations)) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  clock_t start_time = clock();
  if (start_time == (clock_t)-1) {
    fprintf(
        stderr,
        "Warning: could not start CPU timing. Pi will still be calculated.\n");
  }

  long double pi = calculate_pi_serial(iterations);

  clock_t end_time = clock();
  if (end_time == (clock_t)-1) {
    fprintf(stderr, "Warning: could not finish CPU timing.\n");
  }

  printf("mode: serial\n");
  printf("iterations: %" PRIu64 "\n", iterations);
  printf("pi: %.18Lf\n", pi);

  double seconds = elapsed_cpu_seconds(start_time, end_time);
  if (seconds >= 0.0) {
    printf("cpu_time_seconds: %.6f\n", seconds);
  }

  return EXIT_SUCCESS;
}