#include "pacman.hpp"

#include <string.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

#include "macros.hpp"

using std::cerr;

int getRand(int min, int max) { return (rand() % (max - min)) + min; }

// Expects empty write spot on 11
inline int checkElement(char element) {
    switch (element) {
        case PACMAN:
            return 0;
        case RED:
            return 1;
        case BLUE:
            return 2;
        case GREEN:
            return 3;
        case YELLOW:
            return 4;
        case FILE1:
            return 5;
        case FILE2:
            return 6;
        case FILE3:
            return 7;
        case FILE4:
            return 8;
        case FILE5:
            return 9;
        case FILE6:
            return 10;
        default:
            return 11;
    }
}

// Grid -------------------------------------------------------------

Grid::Grid(int rows, int cols) {
    this->rows = rows;
    this->cols = cols;
    this->spots = new char[rows * cols];
}

Grid::~Grid() { delete this->spots; }

inline char* Grid::at(int row, int col) {
    return &this->spots[row + col * this->cols];
}

inline char* Grid::at(Vec2 pos) {
    return &this->spots[pos.y + pos.x * this->cols];
}

void Grid::readGrid(const char* filename) {
    // Open File with "filename", with read cursor at the end
    std::ifstream file(filename, std::ios::ate);
    if (!file.is_open()) {
        cerr << "error when reading file \n";
        return;
    }

    // Use tellg on end to ger file size
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (file_size < this->rows * this->cols * 2) return;
    char buffer[file_size];
    file.read(buffer, file_size);

    // Não Tenho certeza sobre esse pedaço, talvez remover e só considerar sem
    // fim de linha no csv
    if (file_size >= this->rows * this->cols * 2) {
        int gridWrite = 0;
        int i = 0;

        // While not EOF
        while (i < file_size) {
            // Read a Row ignoring separators
            for (; i < i + this->rows * 2; i += 2) {
                this->spots[gridWrite] = buffer[i];
                gridWrite++;
            }
            // Ignore \n
            i++;
        }
    } else {
        int gridWrite = 0;
        int i = 0;
        // While not EOF
        while (i < file_size) {
            this->spots[gridWrite] = buffer[i];
            gridWrite++;
            // Jump ','
            i += 2;
        }
    }
    return;
}

void Grid::readGrid(const char* filename, int defPositions[12]) {
    // Open File with "filename", with read cursor at the end
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        cerr << "error when reading file \n";
        return;
    }
    // Use tellg on end to ger file size
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (file_size < this->rows * this->cols * 2 - 1) return;

    char buffer[file_size];
    file.read(buffer, file_size);

    // Não Tenho certeza sobre esse pedaço, talvez remover e só considerar sem
    // fim de linha no csv
    if (file_size >= this->rows * this->cols * 2) {
        int gridWrite = 0;
        int i = 0;

        // While not EOF
        while (i < file_size) {
            // Read a Row ignoring separators
            for (; i < i + this->rows * 2; i += 2) {
                this->spots[gridWrite] = buffer[i];
                defPositions[checkElement(buffer[i])] = gridWrite;
                gridWrite++;
            }
            // Ignore \n
            i++;
        }
    } else {
        int gridWrite = 0;
        int i = 0;
        // While not EOF
        while (i < file_size) {
            this->spots[gridWrite] = buffer[i];
            defPositions[checkElement(buffer[i])] = gridWrite;
            gridWrite++;
            // Jump ','
            i += 2;
        }
    }
    return;
}

// Direction -------------------------------------------------------------

void Direction::pointUp() {
    this->type = up;
    this->v = {.x = 0, .y = -1};
}

void Direction::pointLeft() {
    this->type = left;
    this->v = {.x = -1, .y = 0};
}

void Direction::pointDown() {
    this->type = down;
    this->v = {.x = 0, .y = 1};
}

void Direction::pointRight() {
    this->type = right;
    this->v = {.x = 1, .y = 0};
}

// Pacman -------------------------------------------------------------

Pacman::Pacman() {
    this->position = {5, 5};
    this->visibility = 1;
}

int Pacman::updatePacman(Grid* grid, DirectionType directionPacman) {
    int foundFile;
    Vec2 nextPos = this->position;
    switch (directionPacman) {
        case up:
            nextPos.x--;
            break;
        case down:
            nextPos.x++;
            break;
        case left:
            nextPos.y--;
            break;
        case right:
            nextPos.y++;
            break;
        default:
            return -2;
    }
    if (nextPos.x < 0 || nextPos.x >= grid->cols || nextPos.y < 0 ||
        nextPos.y >= grid->rows)
        return -2;

    char nextSquare = *(grid->at(nextPos));

    // Game Over
    if (nextSquare == RED || nextSquare == BLUE || nextSquare == GREEN ||
        nextSquare == YELLOW)
        return -1;

    switch (nextSquare) {
        case WALL:
            // Decide if invalid input moves ghost or if only re-ask for input
            return -2;
            break;
        case FILE1:
            foundFile = 1;
            break;
        case FILE2:
            foundFile = 2;
            break;
        case FILE3:
            foundFile = 3;
            break;
        case FILE4:
            foundFile = 4;
            break;
        case FILE5:
            foundFile = 5;
            break;
        case FILE6:
            foundFile = 6;
            break;
        default:
            foundFile = 0;
            break;
    }

    *(grid->at(this->position)) = EMPTY;
    this->position = nextPos;
    *(grid->at(nextPos)) = PACMAN;

    return foundFile;
}
// Ghost ------------------------------------------------------------------

int Ghost::updateRed(Grid* grid) {
    bool valid_next_position = false;
    Vec2 next_pos;
    do {
        next_pos = {
            .x = this->position.x + this->direction.v.x,
            .y = this->position.y + this->direction.v.y,
        };
        char next_spot = grid->spots[next_pos.x + next_pos.y * grid->cols];

        switch (this->direction.type) {
            case DirectionType::up:
                if (next_spot != EMPTY) {
                    this->direction.pointLeft();
                } else {
                    valid_next_position = true;
                }
                break;

            case DirectionType::left:
                if (next_spot != EMPTY) {
                    this->direction.pointDown();
                } else {
                    valid_next_position = true;
                }
                break;

            case DirectionType::down:
                if (next_spot != EMPTY) {
                    this->direction.pointRight();
                } else {
                    valid_next_position = true;
                }
                break;

            case DirectionType::right:
                if (next_spot != EMPTY) {
                    this->direction.pointUp();
                } else {
                    valid_next_position = true;
                }
                break;
        }
    } while (!valid_next_position);

    *grid->at(this->position.y, this->position.x) = EMPTY;
    this->position = next_pos;
    *grid->at(this->position.y, this->position.x) = this->type;

    return 0;
};

int Ghost::updateBlue(Grid* grid) {
    bool valid_next_position = false;
    Vec2 next_pos;
    do {
        next_pos = {
            .x = this->position.x + this->direction.v.x,
            .y = this->position.y + this->direction.v.y,
        };
        char next_spot = grid->spots[next_pos.x + next_pos.y * grid->cols];

        switch (this->direction.type) {
            case DirectionType::up:
                if (next_spot != EMPTY) {
                    this->direction.pointRight();
                } else {
                    valid_next_position = true;
                }
                break;

            case DirectionType::right:
                if (next_spot != EMPTY) {
                    this->direction.pointDown();
                } else {
                    valid_next_position = true;
                }
                break;

            case DirectionType::down:
                if (next_spot != EMPTY) {
                    this->direction.pointLeft();
                } else {
                    valid_next_position = true;
                }
                break;

            case DirectionType::left:
                if (next_spot != EMPTY) {
                    this->direction.pointUp();
                } else {
                    valid_next_position = true;
                }
                break;
        }
    } while (!valid_next_position);

    *grid->at(this->position.y, this->position.x) = EMPTY;
    this->position = next_pos;
    *grid->at(this->position.y, this->position.x) = this->type;

    return 0;
};

int Ghost::updateGreen(Grid* grid) {
    if (this->option % 2 == 0) {
        this->updateRed(grid);
    } else {
        this->updateBlue(grid);
    }
    this->option++;

    return 0;
};

int Ghost::updateYellow(Grid* grid) {
    bool valid_next_position = false;
    Vec2 next_pos;
    do {
        int choice = rand() % 4;  // up, left, down, right
        switch (choice) {
            case up:
                this->direction.pointUp();
                break;
            case left:
                this->direction.pointLeft();
                break;
            case down:
                this->direction.pointDown();
                break;
            case right:
                this->direction.pointRight();
                break;
        }
        next_pos = {
            .x = this->position.x + this->direction.v.x,
            .y = this->position.y + this->direction.v.y,
        };
        char next_spot = grid->spots[next_pos.x + next_pos.y * grid->cols];

        if (next_spot == EMPTY) {
            valid_next_position = true;
        }

    } while (!valid_next_position);

    *grid->at(this->position.y, this->position.x) = EMPTY;
    this->position = next_pos;
    *grid->at(this->position.y, this->position.x) = this->type;

    return 0;
};

// Game State --------------------------------------------------------------

GameState::GameState(const char* mapFile) {
    if (!mapFile) return;

    int posDefined[12] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    Vec2 genPosition;

    this->ghost[0].type = RED;
    this->ghost[1].type = BLUE;
    this->ghost[2].type = GREEN;
    this->ghost[3].type = YELLOW;

    this->grid = new Grid(ROWS, COLS);
    this->grid->readGrid(mapFile, posDefined);

    // Check if Pacman already defined
    if (posDefined[0] == -1) {
        do {
            genPosition.x = getRand(0, COLS);
            genPosition.y = getRand(0, ROWS);
        } while (*this->grid->at(genPosition) != EMPTY);

        this->pacman.position = genPosition;
        *this->grid->at(genPosition) = PACMAN;
    } else {
        this->pacman.position.x = posDefined[0] / ROWS;
        this->pacman.position.y = posDefined[0] % ROWS;
    }

    // Check if Ghosts are defined
    for (int i = 0; i < 4; i++) {
        if (posDefined[i + 1] == -1) {
            do {
                genPosition.x = getRand(0, COLS);
                genPosition.y = getRand(0, ROWS);
            } while (*this->grid->at(genPosition) != EMPTY);

            this->ghost[i].position = genPosition;
            *this->grid->at(genPosition) = this->ghost[i].type;
        } else {
            this->ghost[i].position.x = posDefined[i + 1] / ROWS;
            this->ghost[i].position.y = posDefined[i + 1] % ROWS;
        }

        char randDir = getRand(0, 4);
        switch ((DirectionType)randDir) {
            case left:
                this->ghost[i].direction.pointLeft();
                break;
            case right:
                this->ghost[i].direction.pointRight();
                break;
            case down:
                this->ghost[i].direction.pointDown();
                break;
            case up:
                this->ghost[i].direction.pointUp();
                break;
        }
    }

    for (int i = 0; i < 6; i++) {
        if (posDefined[i + 5] == -1) {
            do {
                genPosition.x = getRand(0, COLS);
                genPosition.y = getRand(0, ROWS);
            } while (*this->grid->at(genPosition) != EMPTY);

            *this->grid->at(genPosition) = FILE1 + i;
        }
    }

    // Max visibility determined by gridsize
    if (this->grid->cols > this->grid->rows)
        this->maxVisibility = this->grid->cols / 2;
    else
        this->maxVisibility = this->grid->rows / 2;

    this->round = 0;
    this->remaining_pellets = 6;
}

GameState::~GameState() { this->grid->~Grid(); }

int GameState::updateGameState(DirectionType directionPacman) {
    // Return Win Game
    if (this->remaining_pellets == 0) return 7;

    int foundFile = this->pacman.updatePacman(this->grid, directionPacman);

    // Die on Player Move
    if (foundFile == -1) return -1;

    this->ghost[0].updateRed(this->grid);
    this->ghost[1].updateBlue(this->grid);
    this->ghost[2].updateGreen(this->grid);
    this->ghost[3].updateYellow(this->grid);

    // Player died on ghost move
    char check = *this->grid->at(this->pacman.position);
    if (check == RED || check == BLUE || check == GREEN || check == YELLOW)
        return -1;

    // Return Next Square, if it is equal to a file value initiate transfer
    if (foundFile > 0) this->remaining_pellets--;

    // Update round and vilibility
    this->round++;
    if (this->round % 5 == 0)
        if (this->pacman.visibility < this->maxVisibility)
            this->pacman.visibility++;

    return foundFile;
}

void GameState::printGrid() {
    for (int i = 0; i < this->grid->rows; i++) {
        for (int j = 0; i < this->grid->cols; j++) {
            switch(*this->grid->at(i, j)) {
                case WALL:
                    printf("%s #  %s", );
                case PACMAN:
                    printf("P ");
                case EMPTY:
                case RED:
                case BLUE:
                case GREEN:
                case YELLOW:
                case FILE1: 
                case FILE2: 
                case FILE3: 
                case FILE4: 
                case FILE5: 
                case FILE6: 
            }
            printf("\n");
        }
    }
}
/*
char* GameState::readGameGrid(int* GridSize) {
    cerr << "Entered read grid\n";
    char* returnGrid;
    int readAmount = (this -> pacman.visibility + 2) * (this ->
pacman.visibility + 2); returnGrid = new char[readAmount];

    int readPos = ((this -> pacman.position.x - this->pacman.visibility) *
this->grid->rows)
                    + this -> pacman.position.y - this->pacman.visibility;

    int i = 0;
    while (i < readAmount)
    {
        memcpy(&returnGrid[i], &this -> grid->spots[readPos], this ->
pacman.visibility); readPos += this -> grid -> rows; i += this ->
pacman.visibility;
    }

    *GridSize = readAmount;

    return returnGrid;
}
*/
