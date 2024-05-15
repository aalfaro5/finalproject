#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define TOTAL_POINTS 1000000
#define NUM_THREADS 10

int points_inside_circle = 0;
pthread_mutex_t lock;

void* monte_carlo_pi() {
    int points_per_thread = TOTAL_POINTS / NUM_THREADS;

    for (int i = 0; i < points_per_thread; i++) {
        double x = (double)rand() / RAND_MAX; // Random x-coordinate between 0 and 1
        double y = (double)rand() / RAND_MAX; // Random y-coordinate between 0 and 1

        // Check if the point is inside the circle
        if (sqrt(x * x + y * y) <= 1) {
            pthread_mutex_lock(&lock);
            points_inside_circle++;
            pthread_mutex_unlock(&lock);
        }
    }

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];

    // Seed for random number generation
    srand(time(NULL));
    
    struct timespec start, end;
    
    // Get the start time
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    // Initialize mutex
    pthread_mutex_init(&lock, NULL);

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, monte_carlo_pi, NULL);
    }

    // Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    // Get the end time
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate the elapsed time in seconds with 8 decimal places
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Total elapsed time: %.8f seconds\n", elapsed_time);

    // Destroy mutex
    pthread_mutex_destroy(&lock);

    // Estimate pi using the ratio of points inside the circle to total points
    double pi_estimate = 4.0 * points_inside_circle / TOTAL_POINTS;
    printf("Estimate of pi using Monte Carlo simulation with pthreads: %f\n", pi_estimate);

    return 0;
}


