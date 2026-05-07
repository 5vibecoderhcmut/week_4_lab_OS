CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2 -pthread -Iinclude
TARGET = scheduler

SRC = \
	src/main.c \
	src/core/dispatcher.c \
	src/core/queue.c \
	src/core/scheduler.c \
	src/core/worker.c \
	src/metrics/metrics.c \
	src/policies/aging.c \
	src/policies/fifo.c \
	src/policies/priority.c \
	src/policies/sif.c \
	src/utils/helpers.c \
	src/utils/logger.c \
	src/utils/parser.c \
	src/utils/sync.c \
	src/utils/time_utils.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
