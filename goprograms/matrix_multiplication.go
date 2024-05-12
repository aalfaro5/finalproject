package main

import (
	"fmt"
	"math/rand"
	"time"
)

func randomMatrix(rows, cols int, ch chan<- [][]int) {
	matrix := make([][]int, rows)
	rand.Seed(time.Now().UnixNano())

	for i := 0; i < rows; i++ {
		matrix[i] = make([]int, cols)
		for j := 0; j < cols; j++ {
			matrix[i][j] = rand.Intn(10) // Generating random numbers between 0 and 9
		}
	}

	ch <- matrix
}

func multiplyMatrices(matrix1, matrix2 [][]int, result chan<- [][]int, elapsedTime chan<- float64) {
	rows1, cols1 := len(matrix1), len(matrix1[0])
	rows2, cols2 := len(matrix2), len(matrix2[0])

	if cols1 != rows2 {
		panic("Number of columns in the first matrix must be equal to the number of rows in the second matrix")
	}

	resultMatrix := make([][]int, rows1)
	for i := range resultMatrix {
		resultMatrix[i] = make([]int, cols2)
	}

	// Create a channel to communicate partial results
	partialResults := make(chan []int)

	// Start the timer
	startTime := time.Now()

	// Iterate over each row of matrix1
	for i := 0; i < rows1; i++ {
		// Launch a goroutine to compute each row of the result matrix
		go func(row int) {
			for j := 0; j < cols2; j++ {
				sum := 0
				for k := 0; k < cols1; k++ {
					sum += matrix1[row][k] * matrix2[k][j]
				}
				partialResults <- []int{row, j, sum}
			}
		}(i)
	}

	// Aggregate partial results
	for i := 0; i < rows1*cols2; i++ {
		partialResult := <-partialResults
		row, col, value := partialResult[0], partialResult[1], partialResult[2]
		resultMatrix[row][col] = value
	}

	// Stop the timer
	elapsed := time.Since(startTime).Seconds()

	// Send the result back through the channel
	result <- resultMatrix

	// Send elapsed time in seconds with 8 decimal point precision
	elapsedTime <- elapsed
}

func main() {
	var rows1, cols1, rows2, cols2 int

	fmt.Println("Enter dimensions for the first matrix (rows columns):")
	fmt.Scan(&rows1, &cols1)

	fmt.Println("Enter dimensions for the second matrix (rows columns):")
	fmt.Scan(&rows2, &cols2)

	if cols1 != rows2 {
		fmt.Println("Matrix multiplication is not possible. Number of columns in the first matrix must be equal to the number of rows in the second matrix.")
		return
	}

	// Create channels to communicate matrices, result, and elapsed time
	matrix1Chan := make(chan [][]int)
	matrix2Chan := make(chan [][]int)
	resultChan := make(chan [][]int)
	elapsedTimeChan := make(chan float64)

	// Generate random matrices concurrently
	go randomMatrix(rows1, cols1, matrix1Chan)
	go randomMatrix(rows2, cols2, matrix2Chan)

	matrix1 := <-matrix1Chan
	matrix2 := <-matrix2Chan

	fmt.Println("First matrix:")
	printMatrix(matrix1)

	fmt.Println("Second matrix:")
	printMatrix(matrix2)

	// Perform matrix multiplication concurrently
	go multiplyMatrices(matrix1, matrix2, resultChan, elapsedTimeChan)

	result := <-resultChan
	elapsedTime := <-elapsedTimeChan

	fmt.Println("Result of matrix multiplication:")
	printMatrix(result)

	// Print the elapsed time
	fmt.Printf("Matrix multiplication took %.8f seconds\n", elapsedTime)
}

func printMatrix(matrix [][]int) {
	for _, row := range matrix {
		for _, val := range row {
			fmt.Printf("%d ", val)
		}
		fmt.Println()
	}
}

