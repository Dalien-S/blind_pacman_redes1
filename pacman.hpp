#define WALL 'X'
#define PACMAN 'P'
#define EMPTY '0'
#define RED 'R'
#define BLUE 'B'
#define GREEN 'G'
#define YELLOW 'Y'
#define OUTBOUNDS '#'


#define FILE1 '1'
#define FILE2 '2'
#define FILE3 '3'
#define FILE4 '4'
#define FILE5 '5'
#define FILE6 '6'

#define WIN 7
#define LOSE -1

#define COLS 40
#define ROWS 40


struct Vec2 {
    int x, y;
};

struct Grid {
    int rows;
    int cols;
    char* spots;

    Grid(int rows, int cols);

    ~Grid();

    inline char* at(int row, int col);
    inline char* at(Vec2 pos);

    void readGrid(const char* filename);
    void readGrid(const char* filename, int defPositions[12]);
};
typedef struct Grid Grid;

enum DirectionType {
    up = 0,
    left = 1,
    down = 2,
    right = 3,
    invalid = 4,
};
typedef enum DirectionType DirectionType;

struct Direction {
    DirectionType type;
    Vec2 v;

    void pointUp();
    void pointLeft();
    void pointDown();
    void pointRight();
};
typedef struct Direction Direction;

struct Pacman {
    Vec2 position;
    int visibility;

    Pacman();
    int updatePacman(Grid* grid, DirectionType directionPacman);
};
typedef struct Pacman Pacman;

struct Ghost {
    char type;  // 'R', 'G', 'B', 'Y'
    Vec2 position;
    int option;  // used when green decides if uses updateLeft or updateRight
    Direction direction;
    //int (*update)(struct Ghost* ghost, Grid* grid);

    int updateRed(Grid* grid);
    int updateBlue(Grid* grid);
    int updateGreen(Grid* grid);
    int updateYellow(Grid* grid);
};
typedef struct Ghost Ghost;


struct GameState {
    Grid *grid;
    Pacman pacman;
    Ghost ghost[4];
    int remaining_pellets;
    int round;
    int maxVisibility;
    int win;

    GameState(const char* mapFile);
    ~GameState();
    int updateGameState(DirectionType directionPacman);
    char* readGameGrid(int* GridSize, int* center);

    void printGrid();
    void printGridBlind();
};
typedef GameState GameState;

void printGridFromBuffer(const char* buffer, int rows, int cols);
