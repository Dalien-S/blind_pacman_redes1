#include "pacman.hpp"
#include <iostream>
#include <cstdlib>

using std::cerr;
using std::cout;

void printGrid(char* grid, int size, int lineSize){
    //cerr << "Enter Print Grid\n";
    for (int i = 0; i < size; i += lineSize){
        for (int j = 0; j < lineSize; j++){
            //cerr << i+j;
            cerr << grid[i+j];}
        cerr << '\n';
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cout << "Less Arguments than expected\n";
        exit(1);
    }

    //std::srand(25055371);

    GameState game(argv[1]);
    int gameReturn;
    do
    {
        //char* gameGrid;
        //int readChars;
        //gameGrid = game.readGameGrid(&readChars);
        cerr << "PRINTING\n";
        printGrid(game.grid->spots, 49, 7);
        cerr << "FINISH PRINTING\n\n\n";
        char direction;
        DirectionType pacDir;
        cerr << "Reading Input\n";
        std::cin >> direction;
        bool invalid = false;
        while (!invalid) { 
            switch (direction)
            {
            case 'w':
                pacDir = up;
                invalid = true;
                break;
            case 'a':
                pacDir = left;
                invalid = true;
                break;
            case 's':
                pacDir = down;
                invalid = true;
                break;
            case 'd':
                pacDir = right;
                invalid = true;
                break;
            default:
                cerr << "Enter Valid Input\n";
                std::cin >> direction;
                break;
            }
        }

        gameReturn = game.updateGameState(pacDir);
        if (gameReturn > 0)
            cerr << "\n\n\n\nFound File" << gameReturn << "\n\n\n\n\n\n";
    } while (gameReturn != -1 && gameReturn != 7);

    if (gameReturn == 7)
        cerr << "\t\t\tYou Win!!!\n\n";
    if (gameReturn == -1)
        cerr << "\t\t\tYou Lose!!!\n\n";
    
    return 0;
}