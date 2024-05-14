package main

import (
    "bufio"
    "fmt"
    "net"
    "os"
    "sync"
    "time"
)

// Function to read URLs from input files and send them to the URL channel
func requester(files []string, urlChan chan<- string, wg *sync.WaitGroup) {
    defer wg.Done()
    for _, file := range files {
        f, err := os.Open(file)
        if err != nil {
            fmt.Fprintf(os.Stderr, "Error opening input file: %s\n", file)
            continue
        }
        scanner := bufio.NewScanner(f)
        for scanner.Scan() {
            url := scanner.Text()
            urlChan <- url
        }
        f.Close()
        if err := scanner.Err(); err != nil {
            fmt.Fprintf(os.Stderr, "Error reading from input file: %s\n", file)
        }
    }
    close(urlChan)
}

// Function to read URLs from the URL channel, resolve them, and send results to the result channel
func resolver(urlChan <-chan string, resultChan chan<- string, wg *sync.WaitGroup) {
    defer wg.Done()
    for url := range urlChan {
        ips, err := net.LookupHost(url)
        if err != nil {
            resultChan <- fmt.Sprintf("%s,ERROR", url)
        } else {
            for _, ip := range ips {
                resultChan <- fmt.Sprintf("%s,%s", url, ip)
            }
        }
    }
}

// Function to write results to the output file
func writer(outputFile string, resultChan <-chan string, done chan<- bool) {
    f, err := os.Create(outputFile)
    if err != nil {
        fmt.Fprintf(os.Stderr, "Error opening output file: %s\n", outputFile)
        done <- false
        return
    }
    defer f.Close()

    for result := range resultChan {
        fmt.Fprintln(f, result)
    }
    done <- true
}

func main() {
    if len(os.Args) < 3 {
        fmt.Fprintf(os.Stderr, "Usage: %s <input file1> ... <input fileN> <output file>\n", os.Args[0])
        os.Exit(1)
    }

    start := time.Now()

    inputFiles := os.Args[1 : len(os.Args)-1]
    outputFile := os.Args[len(os.Args)-1]

    urlChan := make(chan string, 10)
    resultChan := make(chan string, 10)
    done := make(chan bool)

    var wg sync.WaitGroup

    // Start requester goroutine
    wg.Add(1)
    go requester(inputFiles, urlChan, &wg)

    // Start resolver goroutines
    numResolvers := 10
    for i := 0; i < numResolvers; i++ {
        wg.Add(1)
        go resolver(urlChan, resultChan, &wg)
    }

    // Start writer goroutine
    go writer(outputFile, resultChan, done)

    // Wait for requester and resolvers to finish
    wg.Wait()
    close(resultChan)

    // Wait for writer to finish
    if success := <-done; !success {
        os.Exit(1)
    }

    duration := time.Since(start).Seconds()
    fmt.Printf("Processed and resolved all names in %.8f seconds\n", duration)
}

