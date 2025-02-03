CFLAGS=-Wall -Werror -Wextra -g
CROSS_COMPILE ?=
CC=$(CROSS_COMPILE)gcc
TARGET=aesdsocket

SRC = aesdsocket.c
DEPS = aesdsocket.h
OBJ = $(SRC:.c=.o)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

all: $(TARGET)

default: all

.PHONY: clean default all

clean:
	rm -rf *.o $(TARGET)