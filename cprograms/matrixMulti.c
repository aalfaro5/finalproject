#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_DIM 10

int matrix1[MAX_DIM][MAX_DIM];
int matrix2[MAX_DIM][MAX_DIM];
int result[MAX_DIM][MAX_DIM];
int dimension;

// Structure to hold data for each thread
typedef struct {
    int row_start;
    int row_end;
} ThreadData;

// Function declarations
void *multiply(void *arg);
void fillMatrix(int matrix[MAX_DIM][MAX_DIM]);
void printMatrix(int matrix[MAX_DIM][MAX_DIM]);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <dimension>\n", argv[0]);
        return 1;
    }

    dimension = atoi(argv[1]);

    if (dimension <= 0 || dimension > MAX_DIM) {
        printf("Invalid dimension.\n");
        return 1;
    }

    // Fill matrices with random numbers
    fillMatrix(matrix1);
    fillMatrix(matrix2);

    // Print the input matrices
    printf("Matrix 1:\n");
    printMatrix(matrix1);
    printf("\nMatrix 2:\n");
    printMatrix(matrix2);

    // Create pthread variables
    pthread_t threads[MAX_DIM];

    // Create thread data
    ThreadData thread_data[MAX_DIM];

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

    // Print the result matrix
    printf("\nResult:\n");
    printMatrix(result);

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
void fillMatrix(int matrix[MAX_DIM][MAX_DIM]) {
    for (int i = 0; i < dimension; i++) {
        for (int j = 0; j < dimension; j++) {
            matrix[i][j] = rand() % 10; // Generating random numbers between 0 and 9
        }
    }
}

// Function to print a matrix
void printMatrix(int matrix[MAX_DIM][MAX_DIM]) {
    for (int i = 0; i < dimension; i++) {
        for (int j = 0; j < dimension; j++) {
            printf("%d\t", matrix[i][j]);
        }
        printf("\n");
    }
}

