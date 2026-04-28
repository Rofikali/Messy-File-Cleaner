CC = gcc
CFLAGS = -Wall -O2 -Iinclude
TARGET = bin/file-cleaner
SRC = $(wildcard src/**/*.c src/*.c)

all:
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)
