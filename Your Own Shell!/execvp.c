#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char *arguments[1000];

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [arguments...]\n", argv[0]);
        return 1;
    }

    // Setup arguments
    char *command = argv[1];
    for (int i = 1; i < argc; i++) {
        arguments[i - 1] = argv[i];
    }
    arguments[argc - 1] = NULL;

    printf("Command to be executed: %s\n", command);
    printf("Arguments: ");
    for (int i = 0; i < argc - 1; i++) {
        printf("%s ", arguments[i]);
    }
    printf("\n");

    // Attempt to execute the command
    if (execvp(command, arguments) == -1) {
        perror("execvp failed");
        return 1;
    }

    return 0;
}
