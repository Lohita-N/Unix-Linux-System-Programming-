#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#define MAXLEN 1000

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: imclient <server-fifo> <user-id>\n");
        exit(1);
    }

    char clientfifo[MAXLEN];
    sprintf(clientfifo, "/tmp/%s-%d", getenv("USER"), getpid());
    mkfifo(clientfifo, 0600);
    chmod(clientfifo, 0622);

    // Connect to server FIFO and send clientfifo and username
    FILE *fp = fopen(argv[1], "w");
    if (!fp) {
        perror("Cannot open server FIFO");
        unlink(clientfifo);
        exit(1);
    }

    fprintf(fp, "%s %s\n", clientfifo, argv[2]);
    fclose(fp);

    // Read server's response FIFO path from our own FIFO
    FILE *clientfp = fopen(clientfifo, "r");
    if (!clientfp) {
        perror("Cannot open client FIFO");
        unlink(clientfifo);
        exit(1);
    }

    char serverfifo[MAXLEN], line[MAXLEN];
    if (fscanf(clientfp, "%s", serverfifo) != 1) {
        fprintf(stderr, "Failed to read server FIFO path.\n");
        fclose(clientfp);
        unlink(clientfifo);
        exit(1);
    }
    fgets(line, MAXLEN, clientfp); // flush newline
 FILE *serverfp = fopen(serverfifo, "w");
    if (!serverfp) {
        perror("Cannot open server FIFO for writing");
        fclose(clientfp);
        unlink(clientfifo);
        exit(1);
    }

    puts("You are now connected! Start typing your messages or type 'list'.");

    pid_t childPid = fork();
    if (childPid == 0) {
        // Child: send user input to server
        while (1) {
            if (!fgets(line, MAXLEN, stdin))
                break;
            fprintf(serverfp, "%s\n", line);
            fflush(serverfp);
        }
        exit(0);
    } else {
        // Parent: receive messages from server
        while (1) {
            if (!fgets(line, MAXLEN, clientfp))
                break;
            printf("%s", line); // avoid format string vulnerability
        }
        kill(childPid, SIGTERM);
    }

    fclose(serverfp);
    fclose(clientfp);
    unlink(clientfifo);
    return 0;
}
