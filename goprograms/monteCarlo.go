package main

import (
	"fmt"
	"math/rand"
	"sync"
	"time"
)

// Worker function to generate points and send results through a channel
func worker(id int, samples int, results chan<- int, wg *sync.WaitGroup) {
	defer wg.Done()
	rand.Seed(time.Now().UnixNano() + int64(id))
	count := 0

	for i := 0; i < samples; i++ {
		x := rand.Float64()
		y := rand.Float64()
		if x*x+y*y <= 1 {
			count++
		}
	}
	results <- count
}

func main() {
	start := time.Now() // Start timing

	// Number of Go routines and total samples
	numWorkers := 4
	totalSamples := 1000000

	// Channel for collecting results
	results := make(chan int, numWorkers)

	// WaitGroup to wait for all Go routines to finish
	var wg sync.WaitGroup

	// Launch workers
	samplesPerWorker := totalSamples / numWorkers
	for i := 0; i < numWorkers; i++ {
		wg.Add(1)
		go worker(i, samplesPerWorker, results, &wg)
	}

	// Close the results channel once all workers are done
	go func() {
		wg.Wait()
		close(results)
	}()

	// Collect results
	totalInCircle := 0
	for count := range results {
		totalInCircle += count
	}

	// Calculate Pi
	pi := 4.0 * float64(totalInCircle) / float64(totalSamples)

	// Calculate elapsed time
	elapsed := time.Since(start).Seconds()

	// Print results
	fmt.Printf("Estimated value of Pi: %.8f\n", pi)
	fmt.Printf("Time taken: %.8f seconds\n", elapsed)
}

