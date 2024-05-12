#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

int **matrix1;
int **matrix2;
int **result;
int dimension;

// Structure to hold data for each thread
typedef struct {
    int row_start;
    int row_end;
} ThreadData;

// Function declarations
void *multiply(void *arg);
void fillMatrix(int **matrix);
void printMatrix(int **matrix);
void allocateMatrix(int ***matrix, int rows, int cols);
void freeMatrix(int **matrix, int rows);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <dimension>\n", argv[0]);
        return 1;
    }

    dimension = atoi(argv[1]);

    if (dimension <= 0) {
        printf("Invalid dimension.\n");
        return 1;
    }

    // Allocate memory for matrices
    allocateMatrix(&matrix1, dimension, dimension);
    allocateMatrix(&matrix2, dimension, dimension);
    allocateMatrix(&result, dimension, dimension);

    // Fill matrices with random numbers
    fillMatrix(matrix1);
    fillMatrix(matrix2);

    // Print the input matrices
    printf("Matrix 1:\n");
    printMatrix(matrix1);
    printf("\nMatrix 2:\n");
    printMatrix(matrix2);

    // Start time tracking
    clock_t start_time = clock();

    // Create pthread variables
    pthread_t threads[dimension];

    // Create thread data
    ThreadData thread_data[dimension];

    // Create threads for matrix multiplication
    for (int i = 0; i < dimension; i++) {
        thread_data[i].row_start = i;
        thread_data[i].row_end = i + 1;
        pthread_create(&threads[i], NULL, multiply, &thread_data[i]);
    }

    // Join threads
    for (int i = 0; i < dimension; i++) {
        pthread_join(threads[i], NULL);
    }

    // Stop time tracking
    clock_t end_time = clock();

    // Print the result matrix
    printf("\nResult:\n");
    printMatrix(result);

    // Calculate and print the time taken
    double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("\nTime taken: %f seconds\n", time_taken);

    // Free allocated memory
    freeMatrix(matrix1, dimension);
    freeMatrix(matrix2, dimension);
    freeMatrix(result, dimension);

    return 0;
}

// Function to perform matrix multiplication for a portion of the result matrix
void *multiply(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    for (int i = data->row_start; i < data->row_end; i++) {
        for (int j = 0; j < dimension; j++) {
            result[i][j] = 0;
            for (int k = 0; k < dimension; k++) {
                result[i][j] += matrix1[i][k] * matrix2[k][j];
            }
        }
    }
    pthread_exit(NULL);
}

// Function to fill a matrix with random numbers
void fillMatrix(int **matrix) {
    for (int i = 0; i < dimension; i++) {
        for (int j = 0; j < dimension; j++) {
            matrix[i][j] = rand() % 10; // Generating random numbers between 0 and 9
        }
    }
}

// Function to allocate memory for a matrix
void allocateMatrix(int ***matrix, int rows, int cols) {
    *matrix = (int **)malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++) {
        (*matrix)[i] = (int *)malloc(cols * sizeof(int));
    }
}

// Function to free memory allocated for a matrix
void freeMatrix(int **matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

// Function to print a matrix
void printMatrix(int **matrix) {
    for (int i = 0; i < dimension; i++) {
        for (int j = 0; j < dimension; j++) {
            printf("%d\t", matrix[i][j]);
        }
        printf("\n");
    }
}

