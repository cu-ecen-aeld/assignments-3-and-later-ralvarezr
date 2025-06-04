#Makefile for assignment 2

CC = $(CROSS_COMPILE)gcc

# Source files
SRC = writer.c
OBJ = $(SRC:.c=.o)
TARGET = writer

.PHONY: all
all: $(TARGET)

# Build writer app
$(TARGET): $(OBJ)
	$(CC) -o $@ $^

# Compile .c files to .o files
%.o: %.c
	$(CC) -c $< -o $@

# Clean target
.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ)