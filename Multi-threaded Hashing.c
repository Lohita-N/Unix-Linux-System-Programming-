#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#define MAX_THREADS 1024

typedef struct {
    int id;
    int total_threads;
    const uint8_t* file_data;
    uint64_t file_size;
    uint32_t result;
} ThreadArgs;

void usage(char*);
uint32_t jenkins_one_at_a_time_hash(const uint8_t* key, uint64_t length);
void* thread_func(void* arg);

int main(int argc, char** argv) {
    if (argc != 3) usage(argv[0]);

    const char* filename = argv[1];
    int num_threads = atoi(argv[2]);
    if (num_threads <= 0 || num_threads > MAX_THREADS || (num_threads & (num_threads - 1)) != 0) {
        fprintf(stderr, "Number of threads must be a power of 2 and <= %d\n", MAX_THREADS);
        exit(EXIT_FAILURE);
    }

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        exit(EXIT_FAILURE);
    }

    struct stat fileInfo;
    if (fstat(fd, &fileInfo)) {
        perror("fstat failed");
        exit(EXIT_FAILURE);
    }
uint64_t fileSize = fileInfo.st_size;
    const uint8_t* file_data = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    clock_t start = clock();

    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];

    for (int i = 0; i < num_threads; ++i) {
        args[i].id = i;
        args[i].total_threads = num_threads;
        args[i].file_data = file_data;
        args[i].file_size = fileSize;
        pthread_create(&threads[i], NULL, thread_func, &args[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    uint32_t final_hash = args[0].result;
    clock_t end = clock();
    printf("hash value = %u \n", final_hash);
    printf("time taken = %f \n", (end - start) * 1.0 / CLOCKS_PER_SEC);

    munmap((void*)file_data, fileSize);
    close(fd);
    return EXIT_SUCCESS;
}
void* thread_func(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int id = args->id;
    int total = args->total_threads;
    uint64_t chunk_size = args->file_size / total;
    uint64_t start = id * chunk_size;
    uint64_t end = (id == total - 1) ? args->file_size : start + chunk_size;
    uint64_t actual_size = end - start;
    const uint8_t* base = args->file_data + start;
    //uint64_t chunk_size = args->file_size / total;

    // Leaf node threads calculate their hash directly
    if (2 * id + 1 >= total) {
        args->result = jenkins_one_at_a_time_hash(base, actual_size);
        pthread_exit(NULL);
    }

    // Wait for children to finish (join)
    pthread_t left, right;
    ThreadArgs *left_arg = args + (2 * id + 1);
    ThreadArgs *right_arg = (2 * id + 2 < total) ? args + (2 * id + 2) : NULL;

    pthread_create(&left, NULL, thread_func, left_arg);
    if (right_arg) pthread_create(&right, NULL, thread_func, right_arg);

    pthread_join(left, NULL);
    if (right_arg) pthread_join(right, NULL);

    // Compute own hash
    uint32_t own_hash = jenkins_one_at_a_time_hash(base, chunk_size);
    char combined[100];
    if (right_arg) {
        snprintf(combined, sizeof(combined), "%u%u%u", own_hash, left_arg->result, right_arg->result);
    } else {
        snprintf(combined, sizeof(combined), "%u%u", own_hash, left_arg->result);
    }
    args->result = jenkins_one_at_a_time_hash((uint8_t*)combined, strlen(combined));
    pthread_exit(NULL);
}

uint32_t jenkins_one_at_a_time_hash(const uint8_t* key, uint64_t length) {
    uint64_t i = 0;
    uint32_t hash = 0;
    while (i != length) {
        hash += key[i++];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

void usage(char* s) {
    fprintf(stderr, "Usage: %s filename num_threads\n", s);
    exit(EXIT_FAILURE);
}
