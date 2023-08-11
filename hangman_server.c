
#include <arpa/inet.h> // inet_addr()
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // read(), write(), close()

#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024

typedef struct
{
    int flag;
    int wordLength;
    int numIncorrect;
    char data[BUFFER_SIZE];
} ServerMessage;

typedef struct
{
    char *keyword;
    char *guessedWord;
    int wordLength;
    char *guessedLetters;
    char *incorrectLetters;
    int numIncorrect;
    int numCorrect;
} ClientData;

int main()
{
    // read in the word list from hangman_words.txt
    FILE *wordFile = fopen("hangman_words.txt", "r");
    if (wordFile == NULL)
    {
        perror("Error opening word file");
        exit(1);
    }

    // store the words in an array
    char *words[15];
    char word[20];
    int i = 0;
    while (fscanf(wordFile, "%s", word) != EOF)
    {
        words[i] = malloc(strlen(word));
        strcpy(words[i], word);
        words[i][strlen(word)] = '\0';
        i++;
    }

    // print words
    // for (int j = 0; j < 15; j++)
    // {
    //     // printf("%s\n", words[j]);
    // }

    int serverSocket, clientSockets[MAX_CLIENTS];
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize;
    char buffer[BUFFER_SIZE];

    // Create the server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("Error creating server socket");
        exit(1);
    }

    // Set up server address structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the server socket to the specified address and port
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error binding server socket");
        exit(1);
    }

    // Listen for incoming connections
    if (listen(serverSocket, MAX_CLIENTS) < 0)
    {
        perror("Error listening for connections");
        exit(1);
    }

    printf("Server started. Listening for connections...\n");

    fd_set readfds;
    int maxSocket, activity, valRead, newSocket, numClients = 0;
    ClientData clientData[MAX_CLIENTS];
    int random_number;
    int count_sent = 0;

    // Initialize client sockets
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clientSockets[i] = 0;
    }

    while (1)
    {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add the server socket to the set
        FD_SET(serverSocket, &readfds);
        maxSocket = serverSocket;

        // Add client sockets to the set
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            int socketDescriptor = clientSockets[i];

            if (socketDescriptor > 0)
            {
                FD_SET(socketDescriptor, &readfds);
            }

            if (socketDescriptor > maxSocket)
            {
                maxSocket = socketDescriptor;
            }
        }

        // Wait for activity on any of the sockets
        activity = select(maxSocket + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0)
        {
            perror("Error in select");
            exit(1);
        }

        // Check if the server socket has new connection requests
        if (FD_ISSET(serverSocket, &readfds))
        {
            newSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrSize);
            if (newSocket < 0)
            {
                perror("Error accepting connection");
                exit(1);
            }

            if (numClients < MAX_CLIENTS)
            {
                // printf("New connection, socket fd: %d, ip: %s, port: %d\n", newSocket,
                //        inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

                // Generate a random number between 0 and 14
                random_number = rand() % 15;
                // printf("Random number: %d\n", random_number);

                // Add new socket to the array of client sockets
                for (i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clientSockets[i] == 0)
                    {
                        clientSockets[i] = newSocket;
                        numClients++;

                        printf("%d: message sent\n", count_sent);
                        count_sent++;
                        if (send(newSocket, "server-accepted\n", 16, 0) < 0)
                        {
                            perror("Error sending message to client");
                            exit(1);
                        }

                        // Initialize client data
                        clientData[i].keyword = words[random_number];
                        clientData[i].guessedWord = malloc(strlen(clientData[i].keyword));

                        int j;
                        for (j = 0; j < strlen(clientData[i].keyword); j++)
                        {
                            clientData[i].guessedWord[j] = '_';
                        }
                        clientData[i].guessedWord[j] = '\0';

                        clientData[i].wordLength = strlen(clientData[i].keyword);
                        clientData[i].guessedLetters = malloc(26);
                        clientData[i].incorrectLetters = malloc(26);
                        clientData[i].numIncorrect = 0;
                        clientData[i].numCorrect = 0;
                        break;
                    }
                }
            }
            else
            {
                ServerMessage connectionRejected;
                printf("Maximum number of clients reached. Connection rejected.\n");

                // Send message to client
                printf("%d: message sent\n", count_sent);
                count_sent++;

                connectionRejected.flag = 18;
                strcpy(connectionRejected.data, "server-overloaded\n");
                if (send(newSocket, &connectionRejected, sizeof(connectionRejected), 0) < 0)
                {
                    perror("Error sending message to client");
                    exit(1);
                }
                close(newSocket);
                continue;
            }
        }

        // Check for IO operation on client sockets
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int socketDescriptor = clientSockets[i];

            if (FD_ISSET(socketDescriptor, &readfds))
            {
                valRead = read(socketDescriptor, buffer, BUFFER_SIZE);
                if (valRead == 0)
                {
                    // Client disconnected
                    printf("Client disconnected, socket fd: %d\n", socketDescriptor);
                    close(socketDescriptor);
                    numClients--;
                    clientSockets[i] = 0;
                    break;
                }
                else
                {
                    int clientIndex = i;

                    // Update client's data based on the guess
                    char guess = buffer[0];

                    if (guess != '0')
                    {
                        int found = 0;
                        for (int j = 0; j < strlen(clientData[clientIndex].keyword); j++)
                        {
                            if (clientData[clientIndex].keyword[j] == guess)
                            {
                                clientData[clientIndex].guessedWord[j] = guess;
                                clientData[clientIndex].numCorrect++;
                                found = 1;
                            }
                        }
                        // printf("Guessed word: %s\n", clientData[clientIndex].guessedWord);

                        // Update all guesses
                        char add[2] = {guess, '\0'};
                        // printf("Add: %s\n", add);
                        strcat(clientData[clientIndex].guessedLetters, add);
                        // printf("Guessed letters: %s\n", clientData[clientIndex].guessedLetters);

                        // Update incorrect guesses count
                        if (!found)
                        {
                            clientData[clientIndex].incorrectLetters = strcat(clientData[clientIndex].incorrectLetters, add);
                            clientData[clientIndex].numIncorrect++;
                        }
                    }

                    ServerMessage serverMsg;

                    if (clientData[clientIndex].numCorrect == clientData[clientIndex].wordLength)
                    {
                        // Client won
                        serverMsg.flag = 38 + clientData[clientIndex].wordLength;
                        char *message = malloc(100);

                        strcpy(message, "The word was ");
                        strcat(message, clientData[clientIndex].keyword);
                        strcat(message, "\nYou Win!\nGame Over!\n\0");
                        strcpy(serverMsg.data, message);

                        printf("%d: message sent\n", count_sent);
                        count_sent++;

                        if (send(socketDescriptor, &serverMsg, sizeof(serverMsg), 0) < 0)
                        {
                            perror("Error sending message to client");
                            exit(1);
                        }

                        close(socketDescriptor);
                        numClients--;
                        clientSockets[i] = 0;
                        continue;
                    }
                    else if (clientData[clientIndex].numIncorrect == 6)
                    {
                        // Client lost
                        serverMsg.flag = 39 + clientData[clientIndex].wordLength;
                        char *message = malloc(100);

                        strcpy(message, "The word was ");
                        strcat(message, clientData[clientIndex].keyword);
                        strcat(message, "\nYou Lose!\nGame Over!\n\0");
                        strcpy(serverMsg.data, message);

                        printf("%d: message sent\n", count_sent);
                        count_sent++;

                        if (send(socketDescriptor, &serverMsg, sizeof(serverMsg), 0) < 0)
                        {
                            perror("Error sending message to client");
                            exit(1);
                        }

                        close(socketDescriptor);
                        numClients--;
                        clientSockets[i] = 0;
                        continue;
                    }
                    else
                    {
                        // Process the message and prepare the server message
                        serverMsg.flag = 0;
                        serverMsg.wordLength = clientData[clientIndex].wordLength;
                        serverMsg.numIncorrect = clientData[clientIndex].numIncorrect;
                        char *message = malloc(100);
                        strcpy(message, clientData[clientIndex].guessedWord);
                        strcat(message, clientData[clientIndex].incorrectLetters);
                        strcpy(serverMsg.data, message); // Example data

                        // Print the server message
                        // printf("flag: %d wordLength: %d numIncorrect: %d data: %s\n",
                        //        serverMsg.flag, serverMsg.wordLength, serverMsg.numIncorrect, serverMsg.data);

                        // printf("Sending: %s\n", serverMsg.data);

                        // Send the server message to the client
                        printf("%d: message sent\n", count_sent);
                        count_sent++;

                        if (send(socketDescriptor, &serverMsg, sizeof(serverMsg), 0) < 0)
                        {
                            perror("Error sending message to client");
                            exit(1);
                        }
                    }
                }
            }
        }
    }

    return 0;
}
