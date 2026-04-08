CC = g++
FLAGS = -std=c++17 -Wall -Wextra #-Wno-missing-field-initializers
LIB = raw_sockets.hpp
SRC = raw_sockets.cpp
MAIN = main.cpp
OBJ = main.o raw_sockets.o
TARGET = blind_pacman

all: compile
	$(CC) $(OBJ) -o $(TARGET)

compile: $(SRC) $(LIB)
	$(CC) $(FLAGS) -c $(SRC) $(MAIN)

clean: 
	rm -f $(OBJ) $(TARGET)
