#Makefile for assignment 6

CC ?= $(CROSS_COMPILE)gcc
CFLAGS += -DUSE_AESD_CHAR_DEVICE -I../aesd-char-driver
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

