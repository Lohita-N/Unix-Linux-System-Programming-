#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define PUBLIC_PIPE "/tmp/lxn240002"
#define MAX_WORD_LEN 100

int main() {
    if (mkfifo(PUBLIC_PIPE, 0666) == -1 && errno != EEXIST) {
        perror("Failed to create public FIFO");
        exit(1);
    }

    printf("Server waiting for connections...\n");

    // Open public FIFO for reading (waiting for client requests)
    int fd = open(PUBLIC_PIPE, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open public FIFO");
        exit(1);
    }

    FILE *public_fp = fdopen(fd, "r");
    if (!public_fp) {
        perror("Failed to open public FIFO as FILE stream");
        close(fd);
        exit(1);
    }
char client_fifo[100];
    while (fgets(client_fifo, sizeof(client_fifo), public_fp)) {
        printf("Received client FIFO: %s\n", client_fifo);

        char *cptr = strchr(client_fifo, '\n');
        if (cptr) *cptr = '\0';

        // Create a unique server FIFO
        char server_fifo[100];
        snprintf(server_fifo, sizeof(server_fifo), "/tmp/server_%d", getpid());

        if (mkfifo(server_fifo, 0666) == -1) {
            perror("Failed to create server FIFO");
            continue;
        }

        printf("Created server FIFO: %s\n", server_fifo);

        // Send the server FIFO name back to the client
        FILE *client_fp = fopen(client_fifo, "w");
        if (!client_fp) {
            perror("Failed to open client FIFO for writing");
            continue;
        }
        fprintf(client_fp, "%s\n", server_fifo);
        fflush(client_fp);
        fclose(client_fp);

        // Now communicate with the client through their FIFO
        FILE *server_read_fp = fopen(server_fifo, "r");
        FILE *server_write_fp = fopen(server_fifo, "w");
        if (!server_read_fp || !server_write_fp) {
            perror("Failed to open server FIFO for r/w");
            unlink(server_fifo);
            continue;
        }
// Game Loop
        const char *word = "example";  // Change this to a random word or game logic
        int len = strlen(word);
        char display[MAX_WORD_LEN];
        memset(display, '*', len);
        display[len] = '\0';

        int wrong = 0;
        char buffer[100];

        // Game logic - guessing loop
        while (1) {
            // Print the prompt to the client
            fprintf(server_write_fp, "Enter a letter in word %s >\n", display);
            fflush(server_write_fp);  // Ensure the prompt is sent and flushed

            // Wait for the client to send their guess
            if (!fgets(buffer, sizeof(buffer), server_read_fp)) {
                perror("Failed to get input from client FIFO");
                break;
            }

            printf("Received input from client: %s\n", buffer);

            // Process the guess
            char guess = buffer[0];
            int found = 0, already = 0;
            for (int i = 0; i < len; ++i) {
                if (word[i] == guess) {
                    if (display[i] == guess) already = 1;
                    else {
                        display[i] = guess;
                        found = 1;
                    }
                }
            }
if (already) {
                fprintf(server_write_fp, "%c is already in the word.\n", guess);
            } else if (!found) {
                fprintf(server_write_fp, "%c is not in the word.\n", guess);
                wrong++;
            }

            if (strcmp(word, display) == 0) {
                fprintf(server_write_fp, "The word is %s. You missed %d time%s.\n",
                        word, wrong, wrong == 1 ? "" : "s");
                break;
            }

            fflush(server_write_fp);  // Ensure all changes are sent back to the client
        }

        fclose(server_read_fp);
        fclose(server_write_fp);
        unlink(server_fifo);
    }

    fclose(public_fp);
    unlink(PUBLIC_PIPE);
    return 0;
}
