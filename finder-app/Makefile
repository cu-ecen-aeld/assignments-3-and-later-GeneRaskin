CFLAGS=-Wall -Werror -Wextra -g
CROSS_COMPILE ?=
CC=$(CROSS_COMPILE)gcc
TARGET=writer

SRC = writer.c
OBJ = $(SRC:.c=.o)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

default: $(TARGET)

.PHONY: clean default

clean:
	rm -rf *.o $(TARGET)
