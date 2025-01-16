# Variables
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS =
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = qna

# Build target
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Phony targets
.PHONY: all clean run
