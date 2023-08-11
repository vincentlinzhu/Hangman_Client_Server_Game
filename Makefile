CC = gcc
ARGS = -Wall

all: hangman_server hangman_client

hangman_server: hangman_server.o
	$(CC) $(ARGS) -o hangman_server hangman_server.o

hangman_client: hangman_client.o
	$(CC) $(ARGS) -o hangman_client hangman_client.o

clean:
	rm -f *.o all