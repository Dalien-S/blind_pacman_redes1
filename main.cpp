#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include <iostream>

#include "kermit.hpp"
#include "macros.hpp"
#include "raw_sockets.hpp"

using std::cerr;
using std::cout;

const char* messages[] = {"pang",
                          "\tpeng",
                          "\t\tping",
                          "\t\t\tpong",
                          "\t\t\t\tpung",
                          "ESSA MENSAGEM É PRA SER BEM MAIOR AGORA "
                          "INFINITAMENTE MAIOR DO QUE TODAS AS OUTRAS AGORA"};

int runServer(int socket) {
    unsigned int count = 0;
    while (true) {
        const char* data = messages[count % 6];
        KermitPacket message;
        message.header.sequence = count;
        message.send(socket, PacketType::data, data, strlen(data));

        cerr << "message: " << count << "\n";

        count++;
        if (count % 6 == 0) {
            message.header.init_marker = 8;
            for (int i = 0; i < 15; i++) {
                message.sendPacket(socket);
                cerr << "sent dummy message: " << i << "\n";
            }
            break;
        }
    }

    return 0;
}

/*
int runClient(int socket) {
    // unsigned char sequence = 0;
    while (true) {
        KermitPacket message;

        switch (message.receivePacket(socket)) {
            case PacketError::no_error:
                break;

            default:
                break;
        }

        if (message.header.init_marker != KERMIT_INIT_MARKER) {
            // cerr << "got unrecognized init marker "
            //      << std::bitset<8>(message.header.init_marker)
            //      << " discarding...\n";
            continue;
        }

        // if (message.header.sequence == sequence) {
        //     sequence++;
        // } else {
        //     continue;
        // }

        cout << "received message: \n";
        message.printHeader();
        message.printData();

        // Checking CRC for errors
        if (message.checkCRC() == false) {
            message.header.type = nack;
        } else {
            message.header.type = ack;
        }

        switch (message.sendPacket(socket)) {
            case PacketError::send_error:
                cerr << "error on send()\n";
                exit(1);
                break;

            case PacketError::no_error:
                cerr << "no error when sending message\n";
                break;

            default:
                cerr << "AAAAAAAAAAAAAAAAAAAAAAAA\n";
                exit(1);
                break;
        }
    }
}
*/

int main(int argc, char* argv[]) {
    
    if (argc != 2) {
        cout << "Usage: <program> --client|--user\n";
        exit(1);
    }

    cout << "Hello :)\n";
    
    int socket = cria_raw_socket((char*)"enp5s0");
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
    int mode;

    if (strcmp(argv[1], "--server") == 0) {
        mode = 0;
    } else if (strcmp(argv[1], "--client") == 0) {
        mode = 1;
    } else {
        cout << "unrecognized option";
        return 0;
    }

    // Teste de envio alternando entre cliente e servidor
    for (int i = 0; i < 6; i++)
    {
        if (mode == 0) {
            cerr << "\n\n\nI am Server Now\n";
            KermitPacket message;
            message.send(socket, PacketType::initialize, {0}, 0);
            const char* data = messages[i];
            message.send(socket, PacketType::data, data, strlen(data));
            cerr << "message: " << i << "\n";
            message.header.type = end_transmission;
            message.header.size = 0;
            message.data[0] = {0};
            message.calculateCRC(false, message.data);
            message.sendPacket(socket);
            mode = 1;
        }

        else if (mode == 1) {
            cerr << "\n\n\nI am Client Now\n";
            std::vector<char> buffer;
            KermitPacket aux;
            aux.receive(socket, &buffer);
            cerr << "Received end transmission\n";
            cerr << "\tMessage " << i << " Obtained: ";
            for (char a : buffer)
                cerr << a;
            cerr << "\n";
            mode = 0;
        }
    }
}

/*
int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: <program> --client|--user\n";
        exit(1);
    }

    cout << "Hello :)\n";
    // int socket = cria_raw_socket((char*)"lo");
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
*/