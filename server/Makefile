#Makefile for assignment 6

CC = $(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror -g
LDFLAGS=-lpthread

# Source files
SRC = aesdsocket.c
OBJ = $(SRC:.c=.o)
TARGET = aesdsocket

.PHONY: all
all: $(TARGET)

# Build app
$(TARGET): $(OBJ)
	$(CC) ${CFLAGS} -o $@ $^ $(LDFLAGS)

# Compile .c files to .o files
%.o: %.c
	$(CC) ${CFLAGS} -c $< -o $@

# Clean target
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ)