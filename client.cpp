// compile with: g++ kermit.cpp logging.cpp pacman.cpp raw_sockets.cpp
// client.cpp -o client

#include <pwd.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fstream>
#include <vector>

#include "kermit.hpp"
#include "logging.hpp"
#include "pacman.hpp"
#include "raw_sockets.hpp"

using std::cerr;
using std::cin;

// 5x5 grid
const char small_grid[] = {
    WALL,  PACMAN, EMPTY, RED,  BLUE, GREEN, YELLOW, FILE1, FILE2,
    FILE3, WALL,   WALL,  WALL, WALL, WALL,  WALL,   WALL,  WALL,
    WALL,  WALL,   WALL,  WALL, WALL, WALL,  WALL,
};

KermitPacket packet;

void receiveGrid(int socket, std::vector<char>* buffer) {
    // dummy
    packet.send(socket, PacketType::ack, buffer->data(), 0);
    packet.confirmSend(socket);

    PacketType type = packet.receive(socket, buffer);
    if (type != PacketType::data) {
        cerr << "error when receiving grid: (did not receive "
                "PacketType::data)\n";
        exit(1);
    }

    // dummy
    packet.send(socket, PacketType::ack, buffer->data(), 0);
    packet.confirmSend(socket);
}

void receiveFile(int socket, PacketType type, const std::vector<char>* buffer) {
    std::vector<char> copy;
    std::ofstream f;
    KermitPacket received_file;
    const char* output_suffix;

    for (char c : *buffer) {
        copy.push_back(c);
    }

    switch (type) {
        case txt:
            output_suffix = "_received.txt";
            break;
        case jpg:
            output_suffix = "_received.jpg";
            break;
        case mp4:
            output_suffix = "_received.mp4";
            break;
        default:
            cerr << "did not receive a file as parameter\n";
            exit(1);
    }

    copy.insert(copy.end(), output_suffix,
                output_suffix + strlen(output_suffix));
    copy.push_back(0);

    f.open((const char*)copy.data());
    if (!f.is_open()) {
        cerr << "error when trying to open a new file\n";
        exit(1);
    }

    // dummy
    received_file.send(socket, ack, copy.data(), 0);
    received_file.confirmSend(socket);

    received_file.receive(socket, &copy);

    f.write(copy.data(), copy.size());

    f.close();

    // dummy
    packet.send(socket, ack, copy.data(), 0);
    packet.confirmSend(socket);
}

void openFile(const std::vector<char>* filename, PacketType type) {
    std::vector<char> copy;
    const char* output_suffix;

    for (char c : *filename) {
        copy.push_back(c);
    }

    switch (type) {
        case txt:
            output_suffix = "_received.txt";
            break;
        case jpg:
            output_suffix = "_received.jpg";
            break;
        case mp4:
            output_suffix = "_received.mp4";
            break;
        default:
            cerr << "did not receive a file as parameter\n";
            exit(1);
    }
    copy.insert(copy.end(), output_suffix,
                output_suffix + strlen(output_suffix));
    copy.push_back(0);

    pid_t pid = fork();

    if (pid < 0) {
        cerr << "error when calling fork()\n";
        exit(1);
    }

    struct passwd* pw = getpwnam("dalien");
    if (!pw) exit(1);

    // child process
    if (pid == 0) {
        setgid(pw->pw_gid);
        setuid(pw->pw_uid);
        setenv("HOME", pw->pw_dir, 1);

        cerr << "opening file: (" << (char*)copy.data() << ")\n";

        // filename->push_back(0);  // making sure the name is null-terminated
        char* args[] = {
            (char*)"xdg-open",
            (char*)copy.data(),
            // (char*)"&",
            nullptr,
        };
        execvp("xdg-open", args);

        cerr << "error on execl\n";
        perror("execvp");
        exit(1);
    }

    wait(0);  // I don't think we need to wait for the child process to die
}

int runClient(int socket) {
    Logger client_logger = Logger::initLogger("client.log");
    std::vector<char> buffer;  // auxiliary buffer for storing messages

    bool game_is_running = true;
    char rows = 0;
    char cols = 0;
    do {
        PacketType type = packet.receive(socket, &buffer);
        char dir;

        cerr << "received type: " << (int)type << "\n";
        switch (type) {
            case request_movement:
                cerr << "type a direction\n";
                cin >> dir;
                switch (dir) {
                    case 'w':
                        packet.send(socket, walk_up, buffer.data(), 0);
                        packet.confirmSend(socket);
                        break;

                    case 'a':
                        packet.send(socket, walk_left, buffer.data(), 0);
                        packet.confirmSend(socket);
                        break;

                    case 'd':
                        packet.send(socket, walk_right, buffer.data(), 0);
                        packet.confirmSend(socket);
                        break;

                    case 's':
                        packet.send(socket, walk_down, buffer.data(), 0);
                        packet.confirmSend(socket);
                        break;

                    default:
                        cerr << ":(";
                        exit(1);
                }
                break;

            case visualize:
                rows = buffer.data()[0];
                cols = buffer.data()[1];
                // TODO: change cerr to logger later
                cerr << "rows: " << (int)rows << "\n";
                cerr << "cols: " << (int)cols << "\n";

                receiveGrid(socket, &buffer);
                printGridFromBuffer(buffer.data(), rows, cols);

                break;

            case PacketType::txt:
                cerr << "TXT OUTSIDE\n";
                receiveFile(socket, type, &buffer);
                openFile(&buffer, txt);
                break;

            case jpg:
                cerr << "JPG OUTSIDE\n";
                receiveFile(socket, type, &buffer);
                openFile(&buffer, jpg);
                break;

            case mp4:
                cerr << "MP4 OUTSIDE\n";
                receiveFile(socket, type, &buffer);
                openFile(&buffer, mp4);
                break;

            case end_transmission:
                game_is_running = false;

                packet.send(socket, ack, buffer.data(), 0);
                packet.confirmSend(socket);
                break;

            default:
                cout << "other\n";
        }
    } while (game_is_running);

    Logger::terminateLogger(&client_logger);
    return 0;
}

void sendFile(int socket, const char* filename, PacketType type) {
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
}

void serverTest(int socket) {
    bool game_is_running = true;
    do {
        std::vector<char> buffer;
        KermitPacket packet;

        packet.send(socket, request_movement, buffer.data(), 0);
        packet.confirmSend(socket);

        PacketType type = packet.receive(socket, &buffer);

        switch (type) {
            case walk_up:
                buffer.push_back(5);  // rows
                buffer.push_back(5);  // cols

                packet.send(socket, visualize, buffer.data(), 2);
                packet.confirmSend(socket);

                packet.receive(socket, &buffer);  // dummy

                packet.send(socket, data, small_grid, 5 * 5);
                packet.confirmSend(socket);

                packet.receive(socket, &buffer);  // dummy
                break;

            case walk_left:
                sendFile(socket, "bee_movie.txt", txt);

                packet.receive(socket, &buffer);
                break;

            case walk_right:
                sendFile(socket, "kermit.jpg", jpg);

                packet.receive(socket, &buffer);  // dummy
                break;

            case walk_down:
                packet.send(socket, end_transmission, buffer.data(), 0);
                packet.confirmSend(socket);

                packet.receive(socket, &buffer);  // dummy
                game_is_running = false;
                break;
            default:
                break;
        }
    } while (game_is_running);
}

#ifdef CLIENT
#endif

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: <program> --client|--user\n";
        exit(1);
    }

    cout << "Hello :)\n";

    int socket = cria_raw_socket((char*)"enp3s0");
    if (socket == -1) {
        cerr << "Error when creating socket" << "\n";
        exit(1);
    }
    cerr << "Created socket with file descriptor: " << socket << "\n";

    // setting a timeout for the socket
    const int timeoutMillis = TIMEOUT_MS;
    struct timeval timeout = {
        .tv_sec = timeoutMillis / 1000,
        .tv_usec = (timeoutMillis % 1000) * 1000,
    };

    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout,
                   sizeof(timeout)) == -1) {
        cout << "error on setsockopt for timeout\n";
        exit(1);
    }

    if (strcmp(argv[1], "--server") == 0) {
        setKermitLogger("server.log");

        serverTest(socket);

        unsetKermitLogger();
    } else if (strcmp(argv[1], "--client") == 0) {
        setKermitLogger("client.log");

        runClient(socket);

        unsetKermitLogger();
    } else {
        cout << "unrecognized option";
        return 0;
    }
}
