CC = gcc
CFLAGS = -Wall -pthread
LIBS = -lm
all: monte_carlo_pi matrixMulti multi-lookup

monte_carlo_pi: monte_carlo_pi.c
	$(CC) $(CFLAGS) -o monte_carlo_pi monte_carlo_pi.c $(LIBS)

matrixMulti: matrixMulti.c
	$(CC) $(CFLAGS) -o matrixMulti matrixMulti.c $(LIBS)

multi-lookup: multi-lookup.c
	$(CC) $(CFLAGS) -o multi-lookup multi-lookup.c $(LIBS)

clean:
	rm -f monte_carlo_pi matrixMulti multi-lookup

