#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>

#define MAX_WORDS 1000
#define MAX_WORD_LEN 100
#define BUFFER_SIZE 1024

void pexit(char *errmsg) {
    fprintf (stderr,"%s\n", errmsg) ;
    exit(1);
}

//hangman game function
void playgame(int connfd, char *word) {
    char display[MAX_WORD_LEN];
    char buffer[BUFFER_SIZE];
    int len = strlen(word);
    int wrongGuesses = 0;
    int unexposed = len;

    for (int i = 0; i < len; i++) display[i] = '*';
    display[len] = '\0';

    while (unexposed > 0) {
        snprintf(buffer, sizeof(buffer),
            "\nCurrent word: %s\nEnter a letter in word : ", display);
        write(connfd, buffer, strlen(buffer));

        int n = read(connfd, buffer, sizeof(buffer) - 1);
        if (n <= 0) break;
        buffer[n] = '\0';

        char guess = tolower(buffer[0]);
        int found = 0, repeated = 0;

        for (int i = 0; i < len; i++) {
            if (word[i] == guess) {
                if (display[i] == guess) repeated = 1;
                else {
                    display[i] = guess;
                    unexposed--;
                    found = 1;
                }
            }
        }
        if (repeated) {
            snprintf(buffer, sizeof(buffer), "%c is already in the word.\n", guess);
        } else if (found) {
            snprintf(buffer, sizeof(buffer), "Good guess!\n");
        } else {
            snprintf(buffer, sizeof(buffer), "%c is not in the word.\n", guess);
            wrongGuesses++;
        }
        write(connfd, buffer, strlen(buffer));
    }

    snprintf(buffer, sizeof(buffer),
        "\nThe word is: %s\nYou missed %d times.\n", word, wrongGuesses);
    write(connfd, buffer, strlen(buffer));
}
int main(int argc, char *argv[]) {
    int listenfd = 0, connfd = 0, n;
    struct sockaddr_in serv_addr;
    char buffer[1025];
    char *words[MAX_WORDS];
    int word_count = 0;
    char word_buf[MAX_WORD_LEN];

    // Load words from dictionary file
    FILE *fp = fopen("/home/010/l/lx/lxn240002/linux5/a10/dictionary.txt", "r");
    if (!fp) pexit("dictionary.txt not found");
    while (fscanf(fp, "%s", word_buf) == 1 && word_count < MAX_WORDS) {
        words[word_count++] = strdup(word_buf);
    }
    fclose(fp);

    // Create socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        pexit("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(buffer, 0, sizeof(buffer));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int port = 4999;
    do {
        port++;
        serv_addr.sin_port = htons(port);
    } while (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0);

    printf("bind() succeeds for port #%d\n", port);

    if (listen(listenfd, 5) < 0)
        pexit("listen() error");
int counter = 0;
    while (1) {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        counter++;
        printf("connected to client %d.\n", counter);

        if (fork() > 0) {
            close(connfd); // Parent continues
            continue;
        }

        // Child process: handle game
        close(listenfd); // Child doesn't need listener

        srand(time(NULL) ^ getpid());
        char *chosen_word = words[rand() % word_count];
        playgame(connfd, chosen_word);

        close(connfd);
        exit(0); // Child process exits
    }

    return 0;
}
