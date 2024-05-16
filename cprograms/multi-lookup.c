/*
Student: Alejandro Alfaro
Assignment 6, DNS Resolver, IPC
Last Editted:  5/11/2024
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
//get rid of queue.h, work with shared memory instead
//#include "queue.h"


// Add wait for fork usage
#include <sys/wait.h>
// add types for pid usage
#include <sys/types.h>
// add nman for nmap
#include <sys/mman.h>
#define INPUTFS "%1024s"
#include "util.h"
#define MAX_INPUT_FILES 10
#define THREAD_MAX 10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_NAME_LENGTH 1025
#define BUFFER_SIZE 10
// added in INET6_ADDRSTRLEN for storing ipv6 addresses
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

//get rid of queue_size
//#define QUEUE_SIZE 50

typedef struct {
    //get rid of queue
    //queue hostname_queue;
    //instead put buffer array
    char buffer[MAX_INPUT_FILES][MAX_NAME_LENGTH];
    // add in conditional variables
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    pthread_mutex_t mutex;
    pthread_mutex_t outMutex;
    // Set request flag
    int requesting_done;
    // Add an index for the buffer
    int index;
    // max number of names
    int max_names;
    int total_files;
    FILE** input_file;
    FILE* output_file;
} shared_memory;

void requester_function(shared_memory* shared_mem) {
    char line[MAX_NAME_LENGTH];

    for(int i = 0; i < shared_mem->total_files; i++) {
        while(fscanf(shared_mem->input_file[i], INPUTFS, line) > 0) {
            // Critical section so lock
            pthread_mutex_lock(&shared_mem->mutex);
            //wait until buffer is not full
            while (shared_mem->index == BUFFER_SIZE - 1) {
                pthread_cond_wait(&shared_mem->not_full, &shared_mem->mutex);
            }
            //update buffer placement and copy host name to buffer at current index
            shared_mem->index++;
            strcpy(shared_mem->buffer[shared_mem->index], line);
            // Send signal to the resolver 
            pthread_cond_signal(&shared_mem->not_empty);
            // End of critical section
            pthread_mutex_unlock(&shared_mem->mutex);
        }
        //close at i
        fclose(shared_mem->input_file[i]);
    }
    // Update request flag
    shared_mem->requesting_done = 1;
}

//void* resolver_function(void* arg)
void resolver_function(shared_memory* shared_mem) {
    //declare variables to hold hostnames (line) and ipaddresses (ipaddress)
    char line[MAX_IP_LENGTH];
    char ipaddress[INET6_ADDRSTRLEN];

    while(1){
        //lock for start of critical section
        pthread_mutex_lock(&shared_mem->mutex);
        //wait for the buffer to be filled
        while (shared_mem->index < 0 && !shared_mem->requesting_done){
            pthread_cond_wait(&shared_mem->not_empty, &shared_mem->mutex);
        }
        // if buffer is not empty 
        if (shared_mem->index >= 0){
            // copy host name to line
            strcpy(line, shared_mem->buffer[shared_mem->index]);
            // deincrement 
            shared_mem->index--;
            // Signal the requester
            pthread_cond_signal(&shared_mem->not_full);
            // end of critical
            pthread_mutex_unlock(&shared_mem->mutex);

            //dns lookup with hostname (line), copy ipaddress 
            if(dnslookup(line, ipaddress, sizeof(ipaddress)) == UTIL_FAILURE){
                fprintf(stderr, "dnslookup error: %s\n", line);
                strncpy(ipaddress, "", sizeof(ipaddress));
            }

            // Mutex lock and unlock for writing to output file
            pthread_mutex_lock(&shared_mem->outMutex);
            fprintf(shared_mem->output_file, "%s,%s\n", line, ipaddress);
            pthread_mutex_unlock(&shared_mem->outMutex);
        } 
        else
        {
            // Unlock by default at end of function
            pthread_mutex_unlock(&shared_mem->mutex);
            // exit(0);
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if(argc < 4 || argc > MAX_INPUT_FILES + 3) {
      	fprintf(stderr, "Usage: %s <number of names> <input file1> ... <input fileN> <output file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Struct of bounded buffer for shared memory, initialize with nmap
    shared_memory* shared= (shared_memory*) mmap(NULL, sizeof(shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    //verify/error checking
    if (shared== MAP_FAILED){
        perror("Error on mmap on mutex\n");
        exit(1);
    }
    // initialize the mutex between shared_memory
    pthread_mutexattr_t mutex_thing;
    pthread_mutexattr_init(&mutex_thing);
    pthread_mutexattr_setpshared(&mutex_thing, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(shared->mutex), &mutex_thing);
    pthread_mutex_init(&(shared->outMutex), &mutex_thing);
    // set conditions between shared memory
    pthread_condattr_t conditional;
    pthread_condattr_init(&conditional);
    pthread_condattr_setpshared(&conditional, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&shared->not_empty, &conditional);
    pthread_cond_init(&shared->not_full, &conditional);

    // Declare some pids
    pid_t pid1, pid2;
    // Keep track of statuses of process for children for when parent waits
    int status1, status2;
    // start index at -1 instead of 0
    // Had an issue with output,
    // Example
/*
Error looking up Address: Name or service not known
dnslookup error: sdjjdsaf.com
Error looking up Address: Name or service not known
dnslookup error:
*/
    // Having the starting index at -1 got rid of the blank error
    shared->index = -1;

    // Error checking on output file
    shared->output_file = fopen(argv[(argc-1)], "w");
    if (!shared->output_file){
        perror("Error opening output file");
        return EXIT_FAILURE;
    }
    // Store all files in the buffer
    shared->total_files = (argc - 2);
    // Malloc to reserve memory
    shared->input_file = (FILE**)malloc(shared->total_files * sizeof(FILE*));
    // check if reserved properly
    if (!shared->input_file){
        perror("Error could not resize");
        return EXIT_FAILURE;
    }

    //Error checking input files
    //argc-2 for the count
    for(int i = 0; i < (argc-2); i++){
        //if unable to open then print error
        if (!(shared->input_file[i] = fopen(argv[i+1], "r"))){
            fprintf(stderr, "Error opening input file: #%d\n",(i+1));
        }
    }

    // Start splitting the work with forking
    // Because of the use of shared memory, resolver and requester now take in boundedBuffer 
    // instead of file names.
    // First fork, first child, store id1
    if ((pid1 = fork()) == 0){
        //call resolver
        resolver_function(shared);
        exit(0);
    }
    else{ 
        // Second fork, second child, store id2
        if ((pid2 = fork()) == 0){
            // call resolver
            resolver_function(shared);
            exit(0);
        } 
        else{
            // Parent by default should handle the requester
            requester_function(shared);
            // Wait for children to finish before parent
            waitpid(pid1, &status1, 0);
            waitpid(pid2, &status2, 0);
        } 
    }

    // Clean up with destroy
    pthread_condattr_destroy(&conditional);
    pthread_mutexattr_destroy(&mutex_thing);
    pthread_mutex_destroy(&(shared->mutex));
    pthread_mutex_destroy(&(shared->outMutex));
    pthread_cond_destroy(&(shared->not_full));
    pthread_cond_destroy(&(shared->not_empty));

    //Free memory and close output
    free(shared->input_file);
    fclose(shared->output_file);

    // Exit at end 
    return EXIT_SUCCESS;
}
