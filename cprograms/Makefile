CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
LFLAGS = -pthread -lrt -lm

.PHONY: all clean

all: lookup multi-lookup monte_carlo_pi matrixMulti

# Target for the multi-lookup program
multi-lookup: multi-lookup.o queue.o util.o
	$(CC) $(LFLAGS) $^ -o $@

multi-lookup.o: multi-lookup.c multi-lookup.h
	$(CC) $(CFLAGS) -c $<

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c $<

util.o: util.c util.h
	$(CC) $(CFLAGS) -c $<

# Target for the lookup program
lookup: lookup.o util.o
	$(CC) $(LFLAGS) $^ -o $@

lookup.o: lookup.c
	$(CC) $(CFLAGS) -c $<

# Target for the monte_carlo_pi program
monte_carlo_pi: monte_carlo_pi.c
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

# Target for the matrixMulti program
matrixMulti: matrixMulti.c
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

clean:
	rm -f lookup multi-lookup monte_carlo_pi matrixMulti
	rm -f *.o
	rm -f *~
	rm -f results.txt

