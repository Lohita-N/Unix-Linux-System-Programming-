#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define MAXLEN 1000

//execvp the command user inputs after the program prompts

int main() {
        char *arguments[MAXLEN], line[MAXLEN];

        printf("Enter your command: ");
        fgets(line, MAXLEN, stdin);

        char *tool = strtok(line, " \n");
        arguments[0] = tool;

        int i=1;
        //keep getting tokens (individual words)
        while ((arguments[i] = strtok(NULL, " \n")) != NULL)
                i++;

        printf("Invoking: %s\n", tool);
        printf("Arguments(%d): ", i);
        for(int j=0; j<i; j++)
                printf("%s ", arguments[j]);
        puts("\nExecuting the command now.\n");

        execvp(tool, arguments);
        fputs("I hope you don't see me!\n", stderr);
}
