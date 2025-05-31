#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdbool.h>

#define MAXLEN 1000
#define MAXCLIENTS 100

typedef struct {
    char clientfifo[MAXLEN];
    char username[MAXLEN];
    int clientIndex;
} ClientInfo;

char usernames[MAXCLIENTS][MAXLEN];
FILE *clientfps[MAXCLIENTS]; // write-only streams to clients
int numClients = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *server_child(void *ptr) {
    ClientInfo *info = (ClientInfo *)ptr;
    char serverfifo[MAXLEN];
    FILE *clientfp = clientfps[info->clientIndex];

    // Create unique server FIFO using thread ID
    sprintf(serverfifo, "/tmp/%s-%lu", getenv("USER"), pthread_self());
    mkfifo(serverfifo, 0600);
    chmod(serverfifo, 0622);

    // Send server FIFO path to client
    fprintf(clientfp, "%s\n", serverfifo);
    fflush(clientfp);

    FILE *serverfp = fopen(serverfifo, "r");
    if (!serverfp) {
        fprintf(stderr, "Failed to open server FIFO %s\n", serverfifo);
        pthread_exit(NULL);
    }
char line[MAXLEN];
    while (fgets(line, MAXLEN, serverfp)) {
        char *cmd = strtok(line, " \n");
        if (!cmd) continue;

        if (strcmp(cmd, "list") == 0) {
            pthread_mutex_lock(&lock);
            for (int i = 0; i < numClients; i++) {
                fprintf(clientfp, "%s ", usernames[i]);
            }
            fprintf(clientfp, "\n");
            fflush(clientfp);
            pthread_mutex_unlock(&lock);
        } else if (strcmp(cmd, "send") == 0) {
            char *target = strtok(NULL, " \n");
            char *message = strtok(NULL, "\n");

            if (!target || !message) {
                fprintf(clientfp, "Invalid send command format.\n");
                fflush(clientfp);
                continue;
            }

            pthread_mutex_lock(&lock);
            int found = -1;
            for (int i = 0; i < numClients; i++) {
                if (strcmp(usernames[i], target) == 0) {
                    found = i;
                    break;
                }
            }

            if (found == -1) {
                fprintf(clientfp, "Sorry, %s has not joined yet.\n", target);
            } else {
                fprintf(clientfps[found], "%s says %s\n", info->username, message);
                fprintf(clientfp, "Message sent!\n");
            }
            fflush(clientfp);
            pthread_mutex_unlock(&lock);
        }
    }
 fclose(serverfp);
    unlink(serverfifo);
    fclose(clientfp);
    free(info);
    pthread_exit(NULL);
}
int main() {
    char filename[MAXLEN];
    char line[MAXLEN];

    // Create the main server FIFO
    sprintf(filename, "/tmp/%s-%d", getenv("USER"), getpid());
    mkfifo(filename, 0600);
    chmod(filename, 0622);

    printf("Connect to %s to use IM!\n", filename);

    while (1) {
        FILE *fp = fopen(filename, "r");
        if (!fp) {
            perror("Failed to open main server FIFO");
            exit(1);
        }

        while (fgets(line, MAXLEN, fp)) {
            char *clientfifo = strtok(line, " \n");
            char *username = strtok(NULL, " \n");
            if (!clientfifo || !username) continue;

            pthread_mutex_lock(&lock);
            if (numClients >= MAXCLIENTS) {
                fprintf(stderr, "Maximum clients reached.\n");
                pthread_mutex_unlock(&lock);
                continue;
            }

            FILE *clientfp = fopen(clientfifo, "w");
            if (!clientfp) {
                fprintf(stderr, "Cannot open client FIFO: %s\n", clientfifo);
                pthread_mutex_unlock(&lock);
                continue;
            }

            // Store user info
            strcpy(usernames[numClients], username);
            clientfps[numClients] = clientfp;

            // Prepare info for thread
            ClientInfo *info = malloc(sizeof(ClientInfo));
            strcpy(info->clientfifo, clientfifo);
            strcpy(info->username, username);
            info->clientIndex = numClients;

            numClients++;

            // Create thread
            pthread_t tid;
            pthread_create(&tid, NULL, server_child, info);
            pthread_detach(tid); // Don't wait for thread

            printf("%s joined!\n", username);
            pthread_mutex_unlock(&lock);
        }
}

        fclose(fp);
    }

    unlink(filename);
    return 0;
}
