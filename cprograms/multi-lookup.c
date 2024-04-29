/*
Student: Alejandro Alfaro
Assignment 5, DNS Resolver
Last Editted:  4-28-24
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


// Bounded buffer
typedef struct {
    queue hostname_queue;
    pthread_mutex_t mutex;
    //Added in a flag to use if requesting is done
    int requesting_done;
} BoundedBuffer;

BoundedBuffer buffer;
//pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;

// Requester Function
void* requester_function(void* arg)
{
    // Initialize variables
    char line[MAX_NAME_LENGTH];
    char* filename = (char*)arg;
    FILE* input_file = fopen(filename, "r");
    // Error check of input file
    if (input_file == NULL)
    {
        fprintf(stderr, "Error opening input file: %s\n", filename);
        pthread_exit(NULL);
    }

    // Read lines from input file and store in character array "line"
    while (fgets(line, sizeof(line), input_file) != NULL)
    {
            line[strcspn(line, "\r\n")] = '\0';
            pthread_mutex_lock(&buffer.mutex);
	    // While loop to check that the queue is full
	    while (queue_is_full(&buffer.hostname_queue))
	    {
		    // Sleep for random time, lock and unlock
                    pthread_mutex_unlock(&buffer.mutex);
                    usleep(rand() % 1000);
                    pthread_mutex_lock(&buffer.mutex);
            }
            char* hostname = strdup(line);
	    // Verify that there is a host name before pusing to queue.
       	    if(hostname)
	    {
                    queue_push(&buffer.hostname_queue, hostname);
		    //pthread_cond_signal(&buffer_not_empty); // removed because this is considered conditional
		    pthread_mutex_unlock(&buffer.mutex);
            }
	    pthread_mutex_unlock(&buffer.mutex);
    }
    // Close input file
    fclose(input_file);
    pthread_exit(NULL);
}

// Resolver Function
void* resolver_function(void* arg)
{
    // Initialize variables
    char ip[MAX_IP_LENGTH];
    FILE* output_file = (FILE*)arg; // Cast arg to FILE pointer

    while (1) {
        pthread_mutex_lock(&buffer.mutex);

        
		
	// Check if requesting flag is marked done instead , and still check if the queue is empty
	if (buffer.requesting_done && queue_is_empty(&buffer.hostname_queue)) {
            pthread_mutex_unlock(&buffer.mutex);
            break;
        }
	// Wait until buffer is not empty
	while (queue_is_empty(&buffer.hostname_queue)) {
            pthread_mutex_unlock(&buffer.mutex);
            pthread_mutex_lock(&buffer.mutex);
            if (buffer.requesting_done && queue_is_empty(&buffer.hostname_queue)) {
                pthread_mutex_unlock(&buffer.mutex);
                break;
            }
            //removed conditional here //pthread_cond_wait(&buffer_not_empty, &buffer.mutex);
        }
	
        // Pop the next hostname 
        char* hostname = queue_pop(&buffer.hostname_queue);
        pthread_mutex_unlock(&buffer.mutex);
        // Check that we have a hostname to work with
        if (hostname != NULL) {
            // Do the DNS lookup, if fails then print to terminal the error
            if (dnslookup(hostname, ip, sizeof(ip)) == UTIL_FAILURE) {
                fprintf(stderr, "Error resolving hostname: %s\n", hostname);
                ip[0] = '\0';
            }
            // Print to file and free hostname for the next hostname
            // Add lock since writing to a file is also a critical section
            pthread_mutex_lock(&buffer.mutex);
            fprintf(output_file, "%s,%s\n", hostname, ip);
            pthread_mutex_unlock(&buffer.mutex);
            free(hostname);
        }
    }
	pthread_mutex_unlock(&buffer.mutex);
    // Exit the thread
    pthread_exit(NULL);
}
int main(int argc, char *argv[]) {
    //Error checking on arguments
    if (argc < 3 || argc > MAX_INPUT_FILES + 2) {
        fprintf(stderr, "Usage: %s <input file1> ... <input fileN> <output file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Open Output file
    FILE* output_file = fopen(argv[argc - 1], "w");
    if (output_file == NULL) {
        fprintf(stderr, "Error opening output file: %s\n", argv[argc - 1]);
        exit(EXIT_FAILURE);
    }
    // Initialize the queue with buffer "buffer"
    if (queue_init(&buffer.hostname_queue, QUEUE_SIZE) == QUEUE_FAILURE) {
        fprintf(stderr, "Error initializing the queue\n");
        fclose(output_file);
        exit(EXIT_FAILURE);
    }


    // Initialize Pthreads, mutex
    pthread_mutex_init(&buffer.mutex, NULL);
    pthread_t requester_threads[argc - 2];
    // Initialize the number of resolver threads with the Max allowed threads, in the case of instructions 10 threads
    int num_resolver_threads = THREAD_MAX;
    if (num_resolver_threads > MAX_RESOLVER_THREADS)
    {
        num_resolver_threads = MAX_RESOLVER_THREADS;
    }
    if (num_resolver_threads < MIN_RESOLVER_THREADS)
    {
        num_resolver_threads = MIN_RESOLVER_THREADS;
    }
    // Create a thread for each requester
    for (int i = 1; i < argc - 1; i++) {
        pthread_create(&requester_threads[i - 1], NULL, requester_function, argv[i]);
    }

    // Create a thread for each resolver
    pthread_t resolver_threads[num_resolver_threads];
    for (int i = 0; i < num_resolver_threads; i++)
    {
        pthread_create(&resolver_threads[i], NULL, resolver_function, output_file);
    }
    
    // Join requester threads and resolver threads
    for (int i = 0; i < argc - 2; i++)
    {
        pthread_join(requester_threads[i], NULL);
    }
buffer.requesting_done = 1;

    for (int i = 0; i < num_resolver_threads; i++) {
        pthread_join(resolver_threads[i], NULL);
    }

    // Close output file, cleanup the queue, and destroy 
    fclose(output_file);
    queue_cleanup(&buffer.hostname_queue);
    pthread_mutex_destroy(&buffer.mutex);

    // Exit 
    return EXIT_SUCCESS;
}

