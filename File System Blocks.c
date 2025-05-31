#include <stdio.h>;
#include <stdlib.h>;
#include <sys/types.h>;
#include <sys/stat.h>;
#include <fcntl.h>;
#include <unistd.h>;
#include <string.h>;
#include <math.h>;

long computeOverheadBlocks(long diskblocks) {
    if(diskblocks > 12){
        return 1;
    }
        return 0;
    }
    int main(int argo, char *argv[]) {
        if (argc != 2) {
        printf("Usage: diskblocks < file size in KBs>\n");
        return -1;
    }
    //input file size is in KB..
    long filesize = atol (argv[1]);
    long diskblocks = filesize / 8;
    if (filesize % 8)
    diskblocks++;
    //Alternate approach is to use ceil(). diskblocks = ceil(filesize / 8.0);
    printf("%ld %ld\n", diskblocks, computeOverheadBlocks(diskblocks) );
}
