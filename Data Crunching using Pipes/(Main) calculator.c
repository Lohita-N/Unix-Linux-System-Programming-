#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAXLEN 1000
#define MAX_OPERATORS 100

char operators[MAX_OPERATORS];
int fds[MAX_OPERATORS * 2 + 1][2];
int operatorCount = 0;
int numPipes = 0;

void child(int index) {
    // Redirect fds[2*index][0] -> STDIN (operand 1)
    // Redirect fds[2*index+1][0] -> pipe read for operand 2
    // Redirect fds[2*index+2][1] -> STDOUT (result)

    dup2(fds[2 * index][0], 0);     // operand1
    dup2(fds[2 * index + 1][0], 3); // operand2 (custom fd)
    dup2(fds[2 * index + 2][1], 1); // output

    // Close all pipes
    for (int i = 0; i < numPipes; i++) {
        close(fds[i][0]);
        close(fds[i][1]);
    }

    // Decide which executable to run
    char *op = &operators[index];
    if (*op == '+') {
        execlp("./add", "./add", NULL);
    } else if (*op == '-') {
        execlp("./subtract", "./subtract", NULL);
    } else if (*op == '*') {
        execlp("./multiply", "./multiply", NULL);
    } else if (*op == '/') {
        execlp("./divide", "./divide", NULL);
    }

    // If exec fails
    fprintf(stderr, "exec failed for operator %c\n", *op);
    exit(1);
}
int main(int argc, char *argv[]) {
    char line[MAXLEN], *temp;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    FILE *dataFile = fopen(argv[1], "r");
    if (!dataFile) {
        perror("Failed to open file");
        exit(1);
    }

    // Read the expression configuration
    fgets(line, MAXLEN, dataFile);
    strtok(line, " \n");  // skip 'a'
    while ((temp = strtok(NULL, " \n"))) {
        operators[operatorCount++] = temp[0];  // + - * /
        strtok(NULL, " \n"); // skip next variable like b, c, etc.
    }

    // Create pipes
    numPipes = operatorCount * 2 + 1;
    for (int i = 0; i < numPipes; i++) {
        if (pipe(fds[i]) == -1) {
            perror("Pipe creation failed");
            exit(1);
        }
    }

    // Create children
    for (int i = 0; i < operatorCount; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            child(i);  // child(i) calls execlp()
        } else if (pid < 0) {
            perror("fork failed");
            exit(1);
        }
    }
// Read data lines and feed operands
    while (fgets(line, MAXLEN, dataFile)) {
        int operands[MAX_OPERATORS + 1];
        int count = 0;
        char *tok = strtok(line, " \n");
        while (tok && count <= operatorCount) {
            operands[count++] = atoi(tok);
            tok = strtok(NULL, " \n");
        }

        // Write first operand to fds[0]
        write(fds[0][1], &operands[0], sizeof(int));

        // Write remaining operands to pipes: fds[2*i+1]
        for (int i = 0; i < operatorCount; i++) {
            write(fds[2 * i + 1][1], &operands[i + 1], sizeof(int));
        }

        // Read final result from last pipe: fds[numPipes - 1][0]
        int result;
        read(fds[numPipes - 1][0], &result, sizeof(int));
        printf("%d\n", result);
    }

    // Close all pipe ends in parent
    for (int i = 0; i < numPipes; i++) {
        close(fds[i][0]);
        close(fds[i][1]);
    }

    // Wait for all children
    for (int i = 0; i < operatorCount; i++) {
        wait(NULL);
    }

    fclose(dataFile);
    return 0;
}
