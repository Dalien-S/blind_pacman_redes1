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

    void readGrid(const char* filePath);
};
typedef struct Grid Grid;

enum DirectionType {
    up = 0,
    left = 1,
    down = 2,
    right = 3,
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
};
typedef struct Pacman Pacman;

struct Ghost {
    char type;  // 'R', 'G', 'B', 'Y'
    Vec2 position;
    int option;  // used when green decides if uses updateLeft or updateRight
    Direction direction;
    void (*update)(struct Ghost* ghost, Grid* grid);

    void updateRed(Grid* grid);
    void updateBlue(Grid* grid);
    void updateGreen(Grid* grid);
    void updateYellow(Grid* grid);
};
typedef struct Ghost Ghost;
