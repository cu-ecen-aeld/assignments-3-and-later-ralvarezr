#Makefile for assignment 5

CC = $(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror -g

# Source files
SRC = aesdsocket.c
OBJ = $(SRC:.c=.o)
TARGET = aesdsocket

.PHONY: all
all: $(TARGET)

# Build app
$(TARGET): $(OBJ)
	$(CC) ${CFLAGS} -o $@ $^

# Compile .c files to .o files
%.o: %.c
	$(CC) ${CFLAGS} -c $< -o $@

# Clean target
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ)