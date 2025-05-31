#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>

void pexit(char *errmsg) {
    fprintf(stderr, "%s\n", errmsg);
    exit(1);
}

#define MAXUSERS 100
#define MAXLEN 100
char userids[MAXUSERS][MAXLEN];
int clientfds[MAXUSERS];
int numUsers = 0;

// Mutex for handling concurrent client access
pthread_mutex_t userLock;

// Function to handle broadcasting a message to all clients
void send_broadcast(const char *msg) {
    if (msg == NULL) return;

    pthread_mutex_lock(&userLock);
    for (int i = 0; i < numUsers; i++) {
        write(clientfds[i], msg, strlen(msg));
        write(clientfds[i], "\n", 1); // To separate each broadcast
    }
    pthread_mutex_unlock(&userLock);
}
// Function to send a message to a specific user
void handle_send(int clientId, const char *targetUserId, const char *msg) {
    if (targetUserId == NULL || msg == NULL) return;

    pthread_mutex_lock(&userLock);
    int userFound = 0;
    for (int i = 0; i < numUsers; i++) {
        if (strcmp(userids[i], targetUserId) == 0) {
            write(clientfds[i], msg, strlen(msg));
            write(clientfds[i], "\n", 1);
            userFound = 1;
            break;
        }
    }
    pthread_mutex_unlock(&userLock);

    if (!userFound) {
        write(clientId, "Sorry, ", 7);
        write(clientId, targetUserId, strlen(targetUserId));
        write(clientId, " has not joined yet.\n", 21);
    } else {
        write(clientId, "msg sent.\n", 10);
    }
}

// Function to handle sending a message to a random client
void send_rand_msg(const char *msg) {
    if (msg == NULL || numUsers == 0) return;

    pthread_mutex_lock(&userLock);
    srand(time(NULL)); // Initialize the random number generator
    int randomIndex = rand() % numUsers; // Select a random client
    write(clientfds[randomIndex], msg, strlen(msg));
    write(clientfds[randomIndex], "\n", 1);
    pthread_mutex_unlock(&userLock);
}

// Function to handle closing a connection for a client
void send_close(int clientId) {
    write(clientId, "You have been disconnected.\n", 29);
    close(clientId);
}
// Function to handle a dedicated server for each client
void *dedicatedServer(void *ptr) {
    int clientId = (int) ptr;
    char buffer[1025];
    int n;

    while ((n = read(clientId, buffer, sizeof(buffer))) > 0) {
        buffer[n] = '\0';

        char *commands = strtok(buffer, " ");
        if (commands == NULL) continue;

        if (strcmp(commands, "broadcast") == 0) {
            // Broadcast msg to all clients
            char *msg = strtok(NULL, "\n");
            send_broadcast(msg);
        }
        else if (strcmp(commands, "send") == 0) {
            // Send msg to a specific user
            char *targetUserId = strtok(NULL, " ");
            char *msg = strtok(NULL, "\n");
            handle_send(clientId, targetUserId, msg);
        }
        else if (strcmp(commands, "random") == 0) {
            // Send msg to a random client
            char *msg = strtok(NULL, "\n");
            send_rand_msg(msg);
        }
        else if (strcmp(commands, "close") == 0) {
            send_close(clientId);
            break;  // Break the loop and the thread ends here
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    char buffer[1025];

    if (pthread_mutex_init(&userLock, NULL) != 0) {
        perror("pthread_mutex_init failed");
        exit(1);
    }
// Create socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() error");
        exit(1);
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(buffer, '0', sizeof(buffer));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int port = 4999;
    do {
        port++;
        serv_addr.sin_port = htons(port);
    } while (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0);
    printf("bind() succeeds for port #%d\n", port);

    if (listen(listenfd, 10) < 0) {
        perror("listen() error");
        exit(1);
    }

    while (1) {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        if (connfd < 0) {
            perror("accept() error");
            continue;
        }
        printf("connected to client %d.\n", numUsers);

        // read the userid here & store userid & connfd in global arrays
        int n = read(connfd, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            close(connfd);
            pthread_exit(NULL);
        }
        buffer[n] = '\0';  // Ensure null termination

        char *userId = strtok(buffer, "\n");

        // Fallback in case userId is NULL
pthread_mutex_lock(&userLock);
        if (userId == NULL || strlen(userId) == 0) {
            sprintf(userids[numUsers], "client%d", numUsers);
        } else {
            strncpy(userids[numUsers], userId, MAXLEN);
            userids[numUsers][MAXLEN - 1] = '\0'; // Safety null-termination
        }
        clientfds[numUsers] = connfd;
        numUsers++;
        pthread_mutex_unlock(&userLock);

        // broadcast to all other users that a new user has joined?
        sprintf(buffer, "%s joined.\n", userId);
        pthread_mutex_lock(&userLock);
        // send this to all previous clients!
        for (int i = 0; i < numUsers; i++) {
            write(clientfds[i], buffer, strlen(buffer));
        }
        pthread_mutex_unlock(&userLock);

        // Spawn a new thread to handle the client
        pthread_t tid;
        pthread_create(&tid, NULL, dedicatedServer, (void*)connfd);
    }

    return 0;
}
