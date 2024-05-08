/*
Student: Alejandro Alfaro
Assignment 5, DNS Resolver
Last Editted:  4-26-24
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "queue.h"
#include "util.h"

#define MAX_INPUT_FILES 10
#define THREAD_MAX 10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define QUEUE_SIZE 50

typedef struct {
    queue hostname_queue;
    pthread_mutex_t mutex;
    int requesting_done;
    int names_processed; // Track number of names processed
    int max_names;       // Maximum number of names to process
} BoundedBuffer;

BoundedBuffer buffer;

void* requester_function(void* arg)
{
    char line[MAX_NAME_LENGTH];
    char* filename = (char*)arg;
    FILE* input_file = fopen(filename, "r");

    if (input_file == NULL)
    {
        fprintf(stderr, "Error opening input file: %s\n", filename);
        pthread_exit(NULL);
    }

    while (fgets(line, sizeof(line), input_file) != NULL && buffer.names_processed < buffer.max_names)
    {
        line[strcspn(line, "\r\n")] = '\0';
        pthread_mutex_lock(&buffer.mutex);
        while(queue_is_full(&buffer.hostname_queue))
        {
            pthread_mutex_unlock(&buffer.mutex);
            usleep(rand() % 1000);
            pthread_mutex_lock(&buffer.mutex);
        }
        char* hostname = strdup(line);
        if(hostname)
        {
            queue_push(&buffer.hostname_queue, hostname);
            buffer.names_processed++; // Increment names processed
        }
        pthread_mutex_unlock(&buffer.mutex);
    }

    fclose(input_file);
    pthread_exit(NULL);
}

void* resolver_function(void* arg)
{
    char ip[MAX_IP_LENGTH];
    FILE* output_file = (FILE*)arg;

    while (1) {
        pthread_mutex_lock(&buffer.mutex);
        if (buffer.requesting_done && queue_is_empty(&buffer.hostname_queue)) {
            pthread_mutex_unlock(&buffer.mutex);
            break; // Exit thread if requesting is done and queue is empty
        }
        while (queue_is_empty(&buffer.hostname_queue)) {
            pthread_mutex_unlock(&buffer.mutex);
            if (buffer.requesting_done && queue_is_empty(&buffer.hostname_queue)) {
                pthread_mutex_unlock(&buffer.mutex);
                break; // Exit thread if requesting is done and queue is empty
            }
            // Wait for hostname in the queue
            // Removed conditional here //pthread_cond_wait(&buffer_not_empty, &buffer.mutex);
            pthread_mutex_lock(&buffer.mutex);
        }

        char* hostname = queue_pop(&buffer.hostname_queue);
        pthread_mutex_unlock(&buffer.mutex);
        if (hostname != NULL) {
            if (dnslookup(hostname, ip, sizeof(ip)) == UTIL_FAILURE) {
                fprintf(stderr, "Error resolving hostname: %s\n", hostname);
                ip[0] = '\0';
            }
            pthread_mutex_lock(&buffer.mutex);
            fprintf(output_file, "%s,%s\n", hostname, ip);
            pthread_mutex_unlock(&buffer.mutex);
            free(hostname);
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 4 || argc > MAX_INPUT_FILES + 3) {
        fprintf(stderr, "Usage: %s <number of names> <input file1> ... <input fileN> <output file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    buffer.max_names = atoi(argv[1]); // Set maximum number of names to process
    if (buffer.max_names <= 0) {
        fprintf(stderr, "Invalid number of names.\n");
        exit(EXIT_FAILURE);
    }

    FILE* output_file = fopen(argv[argc - 1], "w");
    if (output_file == NULL) {
        fprintf(stderr, "Error opening output file: %s\n", argv[argc - 1]);
        exit(EXIT_FAILURE);
    }

    if (queue_init(&buffer.hostname_queue, QUEUE_SIZE) == QUEUE_FAILURE) {
        fprintf(stderr, "Error initializing the queue\n");
        fclose(output_file);
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&buffer.mutex, NULL);
    pthread_t requester_threads[argc - 3];

    int num_resolver_threads = THREAD_MAX;
    if (num_resolver_threads > MAX_RESOLVER_THREADS) {
        num_resolver_threads = MAX_RESOLVER_THREADS;
    }
    if (num_resolver_threads < MIN_RESOLVER_THREADS) {
        num_resolver_threads = MIN_RESOLVER_THREADS;
    }

    for (int i = 2; i < argc - 1; i++) {
        pthread_create(&requester_threads[i - 2], NULL, requester_function, argv[i]);
    }

    pthread_t resolver_threads[num_resolver_threads];
    for (int i = 0; i < num_resolver_threads; i++) {
        pthread_create(&resolver_threads[i], NULL, resolver_function, output_file);
    }

    for (int i = 0; i < argc - 3; i++) {
        pthread_join(requester_threads[i], NULL);
    }

    buffer.requesting_done = 1;

    for (int i = 0; i < num_resolver_threads; i++) {
        pthread_join(resolver_threads[i], NULL);
    }

    fclose(output_file);
    queue_cleanup(&buffer.hostname_queue);
    pthread_mutex_destroy(&buffer.mutex);

    return EXIT_SUCCESS;
}

