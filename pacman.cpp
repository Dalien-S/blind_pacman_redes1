#include <string.h>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "logging.hpp"
#include "pacman.hpp"

// Return True if collided with a ghost
#define GHOST_COLLISION(square) (square == RED || square == BLUE || square == GREEN || square == YELLOW)
// Returns True if out of bounds
#define CHECK_BOUNDS(x,y) (x < 0 || x >= grid->rows || y < 0 || y >= grid->cols)

Logger pacman_logger = Logger::initLogger(stdout);

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
    char buffer[file_size];
    file.read(buffer, file_size);
    
    int gridWrite = 0;
    int readPos = 0;
    // While not EOF
    while (readPos < file_size) {
        // Read a Row ignoring separators
        for (int i = readPos; i < this->rows * 2 + readPos; i += 2) {
            this->spots[gridWrite] = buffer[i];
            defPositions[checkElement(buffer[i])] = gridWrite;
            gridWrite++;
        }
        readPos += this->rows * 2 + 2;
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
    this->position = {0, 0};
    this->visibility = 1;
}

int Pacman::updatePacman(Grid* grid, DirectionType directionPacman) {
    int foundFile;
    Vec2 nextPos = this->position;
    switch (directionPacman) {
        case up:
            nextPos.y--;
            break;
        case down:
            nextPos.y++;
            break;
        case left:
            nextPos.x--;
            break;
        case right:
            nextPos.x++;
            break;
        default:
            return -2;
    }

    if (CHECK_BOUNDS(nextPos.x, nextPos.y))
        return -2;

    char nextSquare = *(grid->at(nextPos));

    // Game Over
    if (GHOST_COLLISION(nextSquare))
        return -1;
    else switch (nextSquare) {
        case WALL:
            return -2;
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

    if (*(grid->at(this->position)) == PACMAN)
        *(grid->at(this->position)) = EMPTY;
    
    *(grid->at(nextPos)) = PACMAN;
    this->position = nextPos;

    return foundFile;
}
// Ghost ------------------------------------------------------------------

int Ghost::updateRed(Grid* grid) {
    bool valid_next_position = false;
    int counter = 0;
    Vec2 next_pos;
    do {
        if (counter == 4)
            return 1;
        next_pos = {
            .x = this->position.x + this->direction.v.x,
            .y = this->position.y + this->direction.v.y,
        };

        char next_spot;
        if (CHECK_BOUNDS(next_pos.x, next_pos.y))
            next_spot = WALL;
        else
            next_spot = *grid -> at(next_pos);
        
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
        counter++;
    } while (!valid_next_position);

    *grid->at(this->position.y, this->position.x) = EMPTY;
    this->position = next_pos;
    *grid->at(this->position.y, this->position.x) = this->type;

    return 0;
};

int Ghost::updateBlue(Grid* grid) {
    bool valid_next_position = false;
    int counter = 0;
    Vec2 next_pos;
    do {
        if (counter == 4)
            return 1;
        next_pos = {
            .x = this->position.x + this->direction.v.x,
            .y = this->position.y + this->direction.v.y,
        };

        char next_spot;
        if (CHECK_BOUNDS(next_pos.x, next_pos.y))
            next_spot = WALL;
        else
            next_spot = *grid -> at(next_pos);

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
        counter++;
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
    bool checkUp = false;
    bool checkDown = false;
    bool checkLeft = false;
    bool checkRight = false;


    Vec2 next_pos;
    do {
        if (checkUp && checkDown && checkLeft && checkRight)
            return 1;
        int choice = rand() % 4;  // up, left, down, right
        switch (choice) {
            case up:
                this->direction.pointUp();
                checkUp = true;
                break;
            case left:
                this->direction.pointLeft();
                checkLeft = true;
                break;
            case down:
                this->direction.pointDown();
                checkDown = true;
                break;
            case right:
                this->direction.pointRight();
                checkRight = true;
                break;
        }
        next_pos = {
            .x = this->position.x + this->direction.v.x,
            .y = this->position.y + this->direction.v.y,
        };

        char next_spot;
        if (CHECK_BOUNDS(next_pos.x, next_pos.y))
            next_spot = WALL;
        else
            next_spot = *grid -> at(next_pos);


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
        this->maxVisibility = this->grid->cols;
    else
        this->maxVisibility = this->grid->rows;

    this->round = 0;
    this->remaining_pellets = 6;
}

GameState::~GameState() { this->grid->~Grid(); }

int GameState::updateGameState(DirectionType directionPacman) {
    
    this->ghost[0].updateRed(this->grid);
    this->ghost[1].updateBlue(this->grid);
    this->ghost[2].updateGreen(this->grid);
    this->ghost[3].updateYellow(this->grid);

    int foundFile = this->pacman.updatePacman(this->grid, directionPacman);

    // Die on Player Move
    if (foundFile == -1) return foundFile;

    // Return Next Square, if it is equal to a file value initiate transfer
    if (foundFile > 0) this->remaining_pellets--;
    
    // Return Win Game
    if (this->remaining_pellets == 0) return 7;

    // Update round and vilibility
    this->round++;
    if (this->round % 5 == 0)
        if (this->pacman.visibility < this->maxVisibility)
            this->pacman.visibility++;

    return foundFile;
}

void GameState::printGrid() {
    for (int i = 0; i < this->grid->rows; i++) {
        for (int j = 0; j < this->grid->cols; j++) {
            switch(*this->grid->at(i, j)) {
                case WALL:
                    pacman_logger.printColor(color::white, "# ");
                    break;
                case PACMAN:
                    pacman_logger.printColor(color::yellow, "@ ");
                    break;
                case EMPTY:
                    pacman_logger.print("  ");
                    break;
                case RED:
                    pacman_logger.printColor(color::red, "A ");
                    break;
                case BLUE:
                    pacman_logger.printColor(color::blue, "A ");
                    break;
                case GREEN:
                    pacman_logger.printColor(color::green, "A ");
                    break;
                case YELLOW:
                    pacman_logger.printColor(color::magenta, "A ");
                    break;
                case FILE1: 
                    pacman_logger.printColor(color::yellow, "o ");
                    break;
                case FILE2: 
                    pacman_logger.printColor(color::yellow, "o ");
                    break;
                case FILE3: 
                    pacman_logger.printColor(color::yellow, "o ");
                    break;
                case FILE4: 
                    pacman_logger.printColor(color::yellow, "o ");
                    break;
                case FILE5: 
                    pacman_logger.printColor(color::yellow, "o ");
                    break;
                case FILE6: 
                    pacman_logger.printColor(color::yellow, "o ");
                    break;
            }
        }
        pacman_logger.print("\n");
    }
}

void GameState::printGridBlind() {

    for (int i = this -> pacman.position.y - this -> pacman.visibility; i <= this -> pacman.position.y + this -> pacman.visibility; i++) {
        for (int j = this -> pacman.position.x - this -> pacman.visibility; j <= this -> pacman.position.x + this -> pacman.visibility; j++) {
            if (CHECK_BOUNDS(i, j))
                pacman_logger.printColor(color::white, "# ");
            else switch(*this->grid->at(i, j)) {
                case WALL:
                    pacman_logger.printColor(color::white, "# ");
                    break;
                case PACMAN:
                    pacman_logger.printColor(color::yellow, "@ ");
                    break;
                case EMPTY:
                    pacman_logger.print("  ");
                    break;
                case RED:
                    pacman_logger.printColor(color::red, "A ");
                    break;
                case BLUE:
                    pacman_logger.printColor(color::blue, "A ");
                    break;
                case GREEN:
                    pacman_logger.printColor(color::green, "A ");
                    break;
                case YELLOW:
                    pacman_logger.printColor(color::magenta, "A ");
                    break;
                case FILE1: 
                    pacman_logger.printColor(color::yellow, "o ");
                    break;
                case FILE2: 
                    pacman_logger.printColor(color::yellow, "o ");
                    break;
                case FILE3: 
                    pacman_logger.printColor(color::yellow, "o ");
                    break;
                case FILE4: 
                    pacman_logger.printColor(color::yellow, "o ");
                    break;
                case FILE5: 
                    pacman_logger.printColor(color::yellow, "o ");
                    break;
                case FILE6: 
                    pacman_logger.printColor(color::yellow, "o ");
                    break;
            }
        }
        pacman_logger.print("\n");
    }
}

// Return Char Buffer containing visible map. Buffer has size GridSize, returned through argument. 
char* GameState::readGameGrid(int* GridSize) {
    int readAmount = (this -> pacman.visibility * 2 + 1);
    readAmount *= readAmount;
    char* returnGrid = new char[readAmount];
    int written = 0; 

    for (int i = this -> pacman.position.y - this -> pacman.visibility; i <= this -> pacman.position.y + this -> pacman.visibility; i++) 
        for (int j = this -> pacman.position.x - this -> pacman.visibility; j <= this -> pacman.position.x + this -> pacman.visibility; j++) 
        {
            if (CHECK_BOUNDS(i, j)) returnGrid[written] = WALL;
            else returnGrid[written] = *this -> grid -> at(i, j);
            written++;
        }

    *GridSize = written;

    return returnGrid;
}

