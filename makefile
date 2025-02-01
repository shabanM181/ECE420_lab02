CC = gcc
CFLAGS = -Wall -Wextra -pthread
SRC = main.c
EXEC = main

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(EXEC)