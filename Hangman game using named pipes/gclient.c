#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PUBLIC_PIPE "/tmp/lxn240002"

int main() {
    // Create unique client FIFO
    char client_fifo[100];
    snprintf(client_fifo, sizeof(client_fifo), "/tmp/client_%d", getpid());
    mkfifo(client_fifo, 0666);

    // Send client FIFO name to server
    FILE *public_fp = fopen(PUBLIC_PIPE, "w");
    if (!public_fp) {
        perror("Failed to open public pipe");
        exit(1);
    }
    fprintf(public_fp, "%s\n", client_fifo);
    fflush(public_fp);
    fclose(public_fp);

    // Receive server FIFO name
    FILE *client_fp = fopen(client_fifo, "r");
    if (!client_fp) {
        perror("Failed to open client FIFO for reading");
        exit(1);
    }

    char server_fifo[100];
    if (!fgets(server_fifo, sizeof(server_fifo), client_fp)) {
        perror("Failed to read server FIFO name");
        fclose(client_fp);
        exit(1);
    }
    char *cptr = strchr(server_fifo, '\n');
    if (cptr) *cptr = '\0';
    fclose(client_fp);

    printf("Received server FIFO: %s\n", server_fifo);

    // Open server FIFO for communication
    FILE *server_fp = fopen(server_fifo, "r+");
    if (!server_fp) {
        perror("Failed to open server FIFO");
        exit(1);
    }
char line[256];
    while (fgets(line, sizeof(line), server_fp)) {
        printf("Server says: %s", line);  // Show what the server says

        if (strstr(line, "Enter a")) {
            // Server is asking for input
            char response[100];
            printf("Enter your guess: ");
            scanf("%s", response);
            fprintf(server_fp, "%c\n", response[0]);
            fflush(server_fp);
        }

        if (strstr(line, "The word is")) {
            break;  // Game over
        }
    }

    fclose(server_fp);
    return 0;
}
