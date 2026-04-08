#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <bitset>
#include <iostream>
// #include <string>

#include "raw_sockets.hpp"

#define FONT_RED "\x1B[31m"
#define FONT_NORMAL "\x1B[0m"
#define BUFFER_SIZE 1024
#define KERMIT_INIT_MARKER 0b01111110
#define TIMEOUT_MS 3000 /*3 seconds for timeout*/

using std::cerr;
using std::cout;

const char* messages[] = {"pang", "\tpeng", "\t\tping", "\t\t\tpong", "\t\t\t\tpung"};

struct MessageHeader {
    unsigned char init_marker;
    unsigned char size : 5;
    unsigned char sequence : 6;
    enum MessageType {
        ack = 0,
        nack = 1,
        visualize = 2,
        initialize = 3,
        data = 4,
        txt = 5,
        jpg = 6,
        mp4 = 7,
        walk_right = 10,
        walk_left = 11,
        walk_up = 12,
        walk_down = 13,
        error = 15,
        end_transmission = 16,
    } type : 5;

    void print() {
        cerr << "init_marker: " << std::bitset<8>(this->init_marker) << "\n";
        cerr << "size: " << (int)this->size << "\n";
        cerr << "sequence: " << (int)this->sequence << "\n";
        cerr << "type: " << (int)this->type << "\n";
    }
};
typedef struct MessageHeader MessageHeader;

int runServer(int socket) {
    unsigned char frame[BUFFER_SIZE];

    unsigned int count = 0;
    while (1) {
        const char* message = messages[count % 5];
        MessageHeader header = {
            .init_marker = KERMIT_INIT_MARKER,
            .size = (unsigned char)(strlen(message)),
            .sequence = 0,
            .type = MessageHeader::MessageType::data,
        };
        memcpy(frame, &header, sizeof(MessageHeader));
        memcpy(frame + sizeof(MessageHeader), message, strlen(message));

        size_t frame_size = sizeof(MessageHeader) + header.size;
        if (frame_size <= 14) {
            frame_size += 14 - frame_size;
        }
        if (send(socket, frame, frame_size, 0) == -1) {
            int err = errno;
            cerr << "send() error on runServer(): " << strerror(err) << "(" << err << ")\n";
        }
        count++;
        if (count % 5 == 0) {
            sleep(5);
        }
    }

    return 0;
}

int runClient(int socket) {
    unsigned char buffer[BUFFER_SIZE];

    while (1) {
        MessageHeader header;
        if (recv(socket, buffer, BUFFER_SIZE, 0) == -1) {
            cout << "timed out on recv, continuing...\n";
            continue;
        }
        memcpy(&header, buffer, sizeof(MessageHeader));
        if (header.init_marker != KERMIT_INIT_MARKER) {
            cerr << "got unrecognized init marker "
                 << std::bitset<8>(header.init_marker) << "discarding...\n";
        }

        cout << "received message: \n";
        header.print();
        cout << FONT_RED;
        cout.write((const char*)buffer + sizeof(MessageHeader), header.size)
            << FONT_NORMAL "\n\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: <program> --client|--user\n";
        exit(1);
    }

    cout << "Hello :)\n";
    int socket = cria_raw_socket((char*)"lo");
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
        if (runServer(socket) == -1) {
            cout << "error when executing runServer()\n";
            exit(1);
        }
    } else if (strcmp(argv[1], "--client") == 0) {
        if (runClient(socket) == -1) {
            cout << "error when executing runClient()\n";
        }
    } else {
        cout << "unrecognized option";
    }

    close(socket);
    return 0;
}
