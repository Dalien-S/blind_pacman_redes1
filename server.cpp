#include <string.h>

#include <fstream>
#include <iostream>
#include <vector>

#include "kermit.hpp"
#include "logging.hpp"
#include "pacman.hpp"
#include "raw_sockets.hpp"

#define FILE1NAME "1.txt"
#define FILE2NAME "2.txt"
#define FILE3NAME "3.jpg"
#define FILE4NAME "4.jpg"
#define FILE5NAME "5.mp4"
#define FILE6NAME "6.mp4"

using std::cerr;
using std::cin;
using std::cout;

void requestForMove(int socket) {
    KermitPacket packet;
    packet.send(socket, request_movement, NULL, 0);
    packet.confirmSend(socket);
}

void sendGrid(int socket, GameState* game) {
    KermitPacket packet;
    std::vector<char> buffer;
    int gridSize, center;

    const char* grid = game->readGameGrid(&gridSize, &center);
    int rows = game->pacman.visibility * 2 + 1;
    buffer.resize(3 * sizeof(int));

    memcpy(buffer.data(), &rows, sizeof(int));
    memcpy(buffer.data() + sizeof(int), &rows, sizeof(int));
    memcpy(buffer.data() + 2 * sizeof(int), &center, sizeof(int));

    packet.send(socket, visualize, buffer.data(), buffer.size());
    packet.confirmSend(socket);
    packet.receive(socket, &buffer);  // dummy

    packet.send(socket, data, grid, gridSize);
    packet.confirmSend(socket);
}

void sendFile(int socket, const char* filename, PacketType type) {
    KermitPacket packet;
    std::vector<char> aux;
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        cerr << "couln't open " << filename << "\n";
        exit(1);
    }

    int size = file.tellg();

    char* buff = new char[size];

    file.seekg(0, std::ios::beg);
    if (file.read(buff, size)) {
        cerr << "read the entire file\n";
    } else {
        cerr << "error when reading the file\n";
        exit(1);
    }
    file.close();

    packet.send(socket, type, filename, strlen(filename));
    packet.confirmSend(socket);

    packet.receive(socket, &aux);  // dummy

    packet.send(socket, data, buff, size);
    packet.confirmSend(socket);

    packet.receive(socket, &aux);  // dummy
}

// Win and Lose do Same Thing For Now
void sendWin(int socket) {
    std::vector<char> buffer;
    KermitPacket packet;
    
    buffer.push_back('1');
    packet.send(socket, end_transmission, buffer.data(), 1);
    packet.confirmSend(socket);

    packet.receive(socket, &buffer);  // dummy
}

// Win and Lose do Same Thing For Now
void sendLose(int socket) {
    std::vector<char> buffer;
    KermitPacket packet;

    buffer.push_back('2');
    packet.send(socket, end_transmission, buffer.data(), 1);
    packet.confirmSend(socket);

    packet.receive(socket, &buffer);  // dummy
}

void runServer(int socket, const char* gameFile) {
    // Logger logger = Logger::initLogger("server.log");
    setKermitLogger("server.log");
    std::vector<char> buffer;  // auxiliary buffer for storing messages
    GameState* game;
    KermitPacket packet;
    PacketType type;
    DirectionType pacDir;
    int status;

    if (!gameFile)
    {
        //logger.print("não recebeu arquivo de mapa\n");
        return;
    }
    game = new GameState(gameFile);

    sendGrid(socket, game);
    packet.receive(socket, &buffer);  // dummy
    do {
        requestForMove(socket);
        type = packet.receive(socket, &buffer);
        switch (type) {
            case walk_up:
                pacDir = up;
                break;
            case walk_down:
                pacDir = down;
                break;
            case walk_left:
                pacDir = left;
                break;
            case walk_right:
                pacDir = right;
                break;
            default:
                // logger.print("não recebeu movimento valido\n");
                pacDir = invalid;
        }

        status = game->updateGameState(pacDir);
        sendGrid(socket, game);
        packet.receive(socket, &buffer);

        switch (status) {
            case 1:
                sendFile(socket, FILE1NAME, txt);
                break;
            case 2:
                sendFile(socket, FILE2NAME, txt);
                break;
            case 3:
                sendFile(socket, FILE3NAME, jpg);
                break;
            case 4:
                sendFile(socket, FILE4NAME, jpg);
                break;
            case 5:
                sendFile(socket, FILE5NAME, mp4);
                break;
            case 6:
                sendFile(socket, FILE6NAME, mp4);
                break;
            default:
                break;
        }
    } while (game->win == 0);
    
    // Lose
    if (game->win == -1) {
        sendGrid(socket, game);
        packet.receive(socket, &buffer);
        sendFile(socket, "lose.mp4", mp4);
        // packet.receive(socket, &buffer);
        sendLose(socket);
    } 
    // Win
    else if (game-> win == 1) {
        sendGrid(socket, game);
        packet.receive(socket, &buffer);
        sendWin(socket);
    }

    // Logger::terminateLogger(&logger);
}
