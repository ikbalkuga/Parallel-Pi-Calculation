CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
PTHREAD_FLAGS = -pthread

SERIAL = pi_serial
PARALLEL = pi_parallel
ITERATIONS ?= 1000000000
THREADS ?= 4

.PHONY: all clean run-serial run-parallel benchmark test-errors help

all: $(SERIAL) $(PARALLEL)

$(SERIAL): pi_serial.c
	$(CC) $(CFLAGS) pi_serial.c -o $(SERIAL)

$(PARALLEL): pi_parallel.c
	$(CC) $(CFLAGS) $(PTHREAD_FLAGS) pi_parallel.c -o $(PARALLEL)

run-serial: $(SERIAL)
	./$(SERIAL) $(ITERATIONS)

run-parallel: $(PARALLEL)
	./$(PARALLEL) $(ITERATIONS) $(THREADS)

benchmark: all
	./benchmark.sh $(ITERATIONS)

test-errors: all
	./test_invalid_inputs.sh

clean:
	rm -f $(SERIAL) $(PARALLEL) benchmark_results.md

help:
	@echo "Targets:"
	@echo "  make                         Build serial and parallel programs"
	@echo "  make run-serial ITERATIONS=N Run serial version"
	@echo "  make run-parallel ITERATIONS=N THREADS=T Run parallel version"
	@echo "  make benchmark ITERATIONS=N  Run /usr/bin/time benchmark table"
	@echo "  make test-errors             Run invalid-input/error-handling tests"
	@echo "  make clean                   Remove generated files"
