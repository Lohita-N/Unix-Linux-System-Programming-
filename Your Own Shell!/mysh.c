#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAXQUOTES 10000
#define MAXLEN 1000

char *quotes[MAXQUOTES];
int numQuotes = 0;

void loadQuotes(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open quotes.txt");
        exit(1);
    }

    char buffer[MAXLEN];
    while (fgets(buffer, sizeof(buffer), fp)) {
        quotes[numQuotes] = (char *)malloc(strlen(buffer)+ 1);
        if(quotes[numQuotes] == NULL){
                perror("Memory allocation failed");
                exit(1);
        }
        strcpy(quotes[numQuotes], buffer);
        numQuotes++;
    }
    fclose(fp);
}

void showRandomQuote() {
    if (numQuotes == 0) return;
    int idx = rand() % numQuotes;
    fputs(quotes[idx], stderr);
}
void runCommand(char *command) {
    char *args[MAXLEN];
    int i = 0;
    args[i] = strtok(command, " \n");
    while (args[i] && i < MAXLEN - 1) {
        args[++i] = strtok(NULL, " \n");
    }
    args[i] = NULL;

    execvp(args[0], args);  // Run the command
    perror("execvp failed");
    exit(1);
}

// Command handler for piping
void child(int i, char *commands[], int numCommands, int pipefds[][2]) {
    // If it's not the first command, rewire its stdin to the previous command's pipe
    if (i > 0) {
        dup2(pipefds[i-1][0], 0);  // Read from previous pipe
    }
    // If it's not the last command, rewire its stdout to the current pipe
    if (i < numCommands - 1) {
        dup2(pipefds[i][1], 1);  // Write to next pipe
    }

    // Close all pipes in the child process
    for (int j = 0; j < numCommands - 1; j++) {
        close(pipefds[j][0]);
        close(pipefds[j][1]);
    }

    runCommand(commands[i]);
}
void processLine(char *line) {
    char *pipePtr = strchr(line, '|');  // Does this command have pipes?
    char *equalPtr = strchr(line, '='); // Does this command have '='?

    if (pipePtr) {  // Handle multiple commands with pipes (| cmd1 | cmd2 | cmd3)
        char *commands[MAXLEN];
        int numCommands = 0;

        // Split the input by pipes
        commands[numCommands] = strtok(line, "|");
        while (commands[numCommands]) {
            numCommands++;
            commands[numCommands] = strtok(NULL, "|");
        }

        int pipefds[numCommands - 1][2];  // One less pipe than the number of commands
        for (int i = 0; i < numCommands - 1; i++) {
            pipe(pipefds[i]);
        }

        for (int i = 0; i < numCommands; i++) {
            if (fork() == 0) {
                // Redirect input/output for each child process
                if (i > 0) {
                    dup2(pipefds[i - 1][0], 0);  // Read from previous pipe
                }
                if (i < numCommands - 1) {
                    dup2(pipefds[i][1], 1);  // Write to next pipe
                }

                // Close all pipes in the child process
                for (int j = 0; j < numCommands - 1; j++) {
                    close(pipefds[j][0]);
                    close(pipefds[j][1]);
                }

                // Run the command
                runCommand(commands[i]);
                exit(0);
            }
        }

        // Close all pipes in the parent process
        for (int i = 0; i < numCommands - 1; i++) {
            close(pipefds[i][0]);
            close(pipefds[i][1]);
        }

        // Wait for all child processes to finish
        for (int i = 0; i < numCommands; i++) {
            wait(NULL);
        }
    }
else if (equalPtr) {  // Handle two-way pipe (cmd1 = cmd2)
        char *cmd1 = strtok(line, "=");
        char *cmd2 = strtok(NULL, "\n");

        if (cmd1 && cmd2) {
            int toParent[2], toChild[2];
            pipe(toParent);
            pipe(toChild);

            if (fork() == 0) {
                // Child: runs cmd2
                dup2(toChild[0], 0);
                dup2(toParent[1], 1);
                close(toChild[1]);
                close(toParent[0]);
                runCommand(cmd2);
                exit(0);
            } else {
                // Parent: runs cmd1
                dup2(toParent[0], 0);
                dup2(toChild[1], 1);
                close(toChild[0]);
                close(toParent[1]);
                runCommand(cmd1);
                wait(NULL);  // Wait for the child to finish
            }
        }
    }
    else {  // Simple command, no pipes or equal sign
        runCommand(line);
    }
}
int main() {
    char line[MAXLEN];

    loadQuotes("quotes.txt");

    srand(time(NULL));

    while (1) {
        showRandomQuote();
        fprintf(stderr, "# "); // Show the prompt

        if (!fgets(line, MAXLEN, stdin)) break;

        if (fork() == 0) {
            processLine(line);  // Process the command line in a child process
        }

        wait(NULL);  // Wait for the child to finish
    }

    return 0;
}
