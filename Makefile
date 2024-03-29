# Makefile

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -g

# Source files
SOURCES = proxy.c simpleSocketAPI.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Target executable
TARGET = proxy

# Phony targets
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

update:
	git pull && make && ./$(TARGET)

.PHONY: update
