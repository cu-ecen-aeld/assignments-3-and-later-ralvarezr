#Makefile for assignment 6

CC ?= $(CROSS_COMPILE)gcc
CFLAGS += -Wall -Werror -Wextra -g
LDFLAGS += -lpthread

SRC = aesdsocket.c
OBJ = $(SRC:.c=.o)
TARGET = aesdsocket

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ)

