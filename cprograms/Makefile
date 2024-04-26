CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET =  matrixMulti
SRC = matrixMulti.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

