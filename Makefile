CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -pthread -Iinclude
TARGET = bin/file-cleaner
SRC = $(wildcard src/**/*.c src/*.c)

all:
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)
