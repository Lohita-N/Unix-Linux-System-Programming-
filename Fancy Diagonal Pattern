#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define LINESIZE 16
#define SIZE 256

int main(int argc, char *argv[]){
        if(argc < 2){
                printf("Usage: diagonal <word1> <word2> ...<wordN>\n");
                return -1;
        }
        // Create a file so that 16 rows of empty will appear with od -c command
        int fd = open("diagonal2.out", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
        char space = ' ';
        char block[SIZE];
        int start = 0;

        for(int i = 0; i < SIZE; i++){
                block[i] = space;
        }

        for(int i = 1; i < argc; i++){
        int n = strlen(argv[i]);

        if(i % 2 != 0){
                 for(int j = 0; j < n; j++){
                         block[(start + j + 2)* LINESIZE + j + 2] = argv[i][j];
                }
        }else {
                for(int j = 0; j < n; j++){
                        block[(start + j) * LINESIZE + (LINESIZE - 1 - j)] = argv[i][j];
                }
        }
        start += n + 1;
        }
//      lseek(fd, LINESIZE * i, SEEK_SET);
        write(fd, block, SIZE);
        close(fd);
        printf("%s has been created. Use od -c %s to see the contents.\n");
        return 0;

}
