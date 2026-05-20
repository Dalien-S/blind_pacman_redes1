#include <cstdlib>
#include <iostream>

#include "pacman.hpp"
#include "logging.hpp"

using std::cerr;
using std::cout;

void printGrid(char* grid, int size, int lineSize) {
    // cerr << "Enter Print Grid\n";
    for (int i = 0; i < size; i += lineSize) {
        for (int j = 0; j < lineSize; j++) {
            cerr << grid[i + j];
        }
        cerr << '\n';
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Less Arguments than expected\n";
        exit(1);
    }
    Logger pacman_logger = Logger::initLogger(stdout);
    // std::srand(25055371);
    pacman_logger.printColor(color::white, "\033[2J\033[H");

    GameState game(argv[1]);
    int gameReturn;
    do {
        //char* gameGrid;
        //int readChars;
        //gameGrid = game.readGameGrid(&readChars);
        pacman_logger.print("\033[2J\033[H");
        game.printGridBlind();
        //pacman_logger.print("\n\n\n\nGenerated Array:\n");
        //printGrid(gameGrid, readChars, game.pacman.visibility*2+1);
        //pacman_logger.print("\n\n\n\nFull View:\n");
        //game.printGrid();
        char direction;
        DirectionType pacDir;
        cout << "Reading Input\n";
        std::cin >> direction;
        bool invalid = false;
        while (!invalid) {
            switch (direction) {
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
    } while (gameReturn != -1 && gameReturn != 7);

    pacman_logger.print("\033[2J\033[H");
    game.printGrid();
    if (gameReturn == 7) cout << "\n\n\n\n\t\t\tYou Win!!!\n\n";
    if (gameReturn == -1) cout << "\n\n\n\n\t\t\tYou Lose!!!\n\n";

    return 0;
}
