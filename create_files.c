// ./create_files.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>

#define TOTAL_FILES 100
// #define TOTAL_FILES 1000000
#define NUM_THREADS 8  // Adjust based on your CPU cores

const char *extensions[] = {".jpg", ".jpeg", ".png", ".html", ".js", ".java"};
int num_exts = 6;
int files_created = 0;
pthread_mutex_t lock;

void* create_files(void* arg) {
    int thread_id = *(int*)arg;
    int files_per_thread = TOTAL_FILES / NUM_THREADS;
    int start = thread_id * files_per_thread;
    int end = (thread_id == NUM_THREADS - 1) ? TOTAL_FILES : start + files_per_thread;

    char filename[64];
    for (int i = start; i < end; i++) {
        int ext_idx = rand() % num_exts;
        sprintf(filename, "generated_files/file_%d%s", i, extensions[ext_idx]);
        
        FILE *f = fopen(filename, "w");
        if (f) fclose(f);

        // Update global counter for progress bar
        if (i % 500 == 0) {
            pthread_mutex_lock(&lock);
            files_created += 500;
            pthread_mutex_unlock(&lock);
        }
    }
    return NULL;
}

int main() {
    mkdir("generated_files", 0777);
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    pthread_mutex_init(&lock, NULL);
    
    clock_t start_time = clock();
    srand(time(NULL));

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, create_files, &thread_ids[i]);
    }

    // Progress bar loop
    while (files_created < TOTAL_FILES) {
        float progress = (float)files_created / TOTAL_FILES;
        double elapsed = (double)(clock() - start_time) / CLOCKS_PER_SEC;
        printf("\rProgress: [%.2f%%] | Elapsed: %.2fs", progress * 100, elapsed);
        fflush(stdout);
        usleep(100000); // Update every 100ms
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);
    printf("\nFinished! Created %d files using %d threads.\n", TOTAL_FILES, NUM_THREADS);
    return 0;
}



// gcc -pthread create_files.c -o create_files
// ./create_files
