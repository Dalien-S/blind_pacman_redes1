CC = g++
FLAGS = -std=c++17 -Wall -Wextra #-Wno-missing-field-initializers
LIB = raw_sockets.hpp kermit.hpp
SRC = raw_sockets.cpp kermit.cpp
MAIN = main.cpp
OBJ = main.o raw_sockets.o kermit.o
TARGET = blind_pacman

all: compile
	$(CC) $(OBJ) -o $(TARGET)

compile: $(SRC) $(LIB)
	$(CC) $(FLAGS) -c $(SRC) $(MAIN)

clean purge: 
	rm -f $(OBJ) $(TARGET)
