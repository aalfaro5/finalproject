package main

import (
    "bufio"
    "fmt"
    "os"
)

func main() {
    // Open the input file
    inputFile, err := os.Open("input/large.txt")
    if err != nil {
        fmt.Println("Error:", err)
        return
    }
    defer inputFile.Close()

    // Create a new output file
    outputFile, err := os.Create("output.txt")
    if err != nil {
        fmt.Println("Error:", err)
        return
    }
    defer outputFile.Close()

    // Create a scanner to read from the input file
    scanner := bufio.NewScanner(inputFile)

    // Write the first 300 lines into the output file
    lineCount := 0
    for scanner.Scan() {
        line := scanner.Text()
        _, err := fmt.Fprintln(outputFile, line)
        if err != nil {
            fmt.Println("Error:", err)
            return
        }
        lineCount++
        if lineCount >= 1000 {
            break
        }
    }

    // Check for errors during scanning
    if err := scanner.Err(); err != nil {
        fmt.Println("Error:", err)
        return
    }

    fmt.Println("First 1000 lines copied to output.txt")
}

