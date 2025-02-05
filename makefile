CC = gcc
CFLAGS = -Wall -Wextra -pthread
SRC = main.c
CLIENT_SRC = client.c
ATTACKER_SRC = attacker.c
EXEC = main
CLIENT_EXEC = client
ATTACKER_EXEC = attacker

all: $(EXEC) $(CLIENT_EXEC) $(ATTACKER_EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT_EXEC): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $<

$(ATTACKER_EXEC): $(ATTACKER_SRC)
	$(CC) $(CFLAGS) -o $@ $< -lm  # Link math library for attacker

clean:
	rm -f $(EXEC) $(CLIENT_EXEC) $(ATTACKER_EXEC)