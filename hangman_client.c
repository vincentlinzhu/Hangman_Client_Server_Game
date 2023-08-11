#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

typedef struct
{
    int flag;
    int wordLength;
    int numIncorrect;
    char data[BUFFER_SIZE];
} ServerMessage;

int main(int argc, char *argv[])
{
    char *ip_address = argv[1];
    int port = atoi(argv[2]);

    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    // Create the client socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        perror("Error creating client socket");
        exit(1);
    }

    // Set up server address structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_address, &(serverAddr.sin_addr)) <= 0)
    {
        perror("Invalid server address");
        exit(1);
    }

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error connecting to server");
        exit(1);
    }

    ServerMessage serverMsg;
    // Receive a response from the server
    if (recv(clientSocket, (char *)&serverMsg, sizeof(serverMsg), 0) < 0)
    {
        perror("Error receiving response from server");
        exit(1);
    }

    // Connection Rejected: Too many clients
    if (serverMsg.flag == 18)
    {
        printf(">>>");
        printf("%s", serverMsg.data);
        close(clientSocket);
        exit(1);
    }

    printf(">>>Ready to start game? (y/n): ");

    // y/n to start/reject game
    char user_input[BUFFER_SIZE];
    if (scanf("%s", user_input) == EOF)
    {
        printf("\n");
        close(clientSocket);
        exit(1);
    }

    if (strcmp(user_input, "y") == 0)
    {
        // Send the message to the server
        if (send(clientSocket, "0\0", 1, 0) < 0)
        {
            perror("Error sending message to server");
            exit(1);
        }
    }
    else
    {
        close(clientSocket);
        exit(1);
    }

    int firstTime = 1;

    while (1)
    {
        ServerMessage serverMsg;

        // Receive a response from the server
        if (recv(clientSocket, (char *)&serverMsg, sizeof(serverMsg), 0) < 0)
        {
            perror("Error receiving response from server");
            exit(1);
        }

        // Connection Rejected
        if (serverMsg.flag == 18)
        {
            break;
        }

        if (serverMsg.flag != 0)
        {
            printf(">>>");
            // print >>> before each newline
            for (int i = 0; i < strlen(serverMsg.data); i++)
            {
                printf("%c", serverMsg.data[i]);
                if (serverMsg.data[i] == '\n' && i < strlen(serverMsg.data) - 1)
                {
                    printf(">>>");
                }
            }
            break;
        }

        char guessedWord[BUFFER_SIZE];
        for (int i = 0; i < serverMsg.wordLength; i++)
        {
            guessedWord[i] = serverMsg.data[i];
        }
        guessedWord[serverMsg.wordLength] = '\0';

        char guessedLetters[BUFFER_SIZE];
        for (int i = 0; i < strlen(serverMsg.data) - serverMsg.wordLength; i++)
        {
            guessedLetters[i] = serverMsg.data[i + serverMsg.wordLength];
        }
        guessedLetters[strlen(serverMsg.data) - serverMsg.wordLength] = '\0';

        printf(">>>");
        for (int i = 0; i < strlen(guessedWord); i++)
        {
            printf("%c", guessedWord[i]);
            if (i < strlen(guessedWord) - 1)
            {
                printf(" ");
            }
            else
            {
                printf("\n");
            }
        }

        printf(">>>Incorrect Guesses: ");
        for (int i = 0; i < strlen(guessedLetters); i++)
        {
            printf("%c", guessedLetters[i]);
            if (i < strlen(guessedLetters) - 1)
            {
                printf(" ");
            }
            else
            {
                printf("\n");
            }
        }

        if (strlen(guessedLetters) == 0)
        {
            printf("\n");
        }

        printf(">>>\n");
        printf(">>>Letter to guess: ");

        if (scanf("%s", buffer) == EOF)
        {
            printf("\n");
            break;
        }

        int quit = 0;

        while (strlen(buffer) != 1 ||
               !((buffer[0] >= 'a' && buffer[0] <= 'z') || (buffer[0] >= 'A' && buffer[0] <= 'Z')))
        {
            printf(">>>Error! Please guess one letter.\n");
            printf(">>>Letter to guess: ");
            if (scanf("%s", buffer) == EOF)
            {
                printf("\n");
                quit = 1;
                break;
            }
        }

        if (quit)
        {
            break;
        }

        if (buffer[0] >= 'A' && buffer[0] <= 'Z')
        {
            buffer[0] = buffer[0] + 32;
        }

        // Send the message to the server
        if (send(clientSocket, buffer, strlen(buffer), 0) < 0)
        {
            perror("Error sending message to server");
            exit(1);
        }

        firstTime = 0;
    }

    // Close the client socket
    close(clientSocket);
    return 0;
}
