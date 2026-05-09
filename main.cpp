#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <vector>

#include "kermit.hpp"
// #include "macros.hpp"
#include "logging.hpp"
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
        cerr << "before confirmSend()\n";
        message.confirmSend(socket);
        cerr << "after confirmSend()\n";

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
    int mode;

    if (strcmp(argv[1], "--server") == 0) {
        // const char* file_name = "bee_movie.txt";
        const char* file_name = "kermit.jpg";
        // const char* file_name = "rickroll.mp4";
        // opening a file for sending
        std::ifstream file(file_name, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            cerr << "couldn't open bee_movie.txt\n";
            exit(1);
        }

        int size = file.tellg();

        char* buffer = new char[size];

        file.seekg(0, std::ios::beg);
        if (file.read(buffer, size)) {
            cerr << "successfully read the bee_movie.txt script\n";
        } else {
            cerr << "error when reading the bee_movie.txt script\n";
            exit(1);
        }
        file.close();

        KermitPacket sent_file;
        sent_file.send(socket, jpg, file_name, strlen(file_name));
        sent_file.confirmSend(socket);

        cerr << color::red << "RECEIVING DUMMY MESSAGE\n" << color::normal;
        std::vector<char> dummy;
        sent_file.receive(socket, &dummy);
        cerr << color::red << "RECEIVED DUMMY MESSAGE\n" << color::normal;

        sent_file.send(socket, data, buffer, size);
        sent_file.confirmSend(socket);

    } else if (strcmp(argv[1], "--client") == 0) {
        std::ofstream f;
        std::vector<char> title;
        KermitPacket received_file;
        PacketType type = received_file.receive(socket, &title);
        if (type == txt) {
            const char* received_name = "_received.txt";

            title.insert(title.end(), received_name,
                         received_name + strlen(received_name));

            f.open((const char*)title.data());
            if (!f.is_open()) {
                cerr << "error when trying to open a new file\n";
            }

        } else if (type == jpg) {
            const char* received_name = "_received.jpg";

            title.insert(title.end(), received_name,
                         received_name + strlen(received_name));

            f.open((const char*)title.data());
            if (!f.is_open()) {
                cerr << "error when trying to open a new file\n";
            }

        } else {
            cerr << "received wrong type: expected txt\n";
            exit(1);
        }

        // sending dummy message
        cerr << color::red << "SENDING DUMMY MESSAGE\n" << color::normal;
        received_file.send(socket, data, title.data(), 0);
        received_file.confirmSend(socket);
        cerr << color::red << "SENT DUMMY MESSAGE\n" << color::normal;

        std::vector<char> movie;
        type = received_file.receive(socket, &movie);
        if (type == data) {
            f.write(movie.data(), movie.size());
        } else {
            cerr << "received wrong type: expected data\n";
        }

        f.close();
    } else {
        cout << "unrecognized option";
        return 0;
    }
}

// int main(int argc, char* argv[]) {
//
//     if (argc != 2) {
//         cout << "Usage: <program> --client|--user\n";
//         exit(1);
//     }
//
//     cout << "Hello :)\n";
//
//     int socket = cria_raw_socket((char*)"enp3s0");
//     if (socket == -1) {
//         cerr << "Error when creating socket" << "\n";
//         exit(1);
//     }
//     cerr << "Created socket with file descriptor: " << socket << "\n";
//
//     // setting a timeout for the socket
//     const int timeoutMillis = TIMEOUT_MS;
//     struct timeval timeout = {
//         .tv_sec = timeoutMillis / 1000,
//         .tv_usec = (timeoutMillis % 1000) * 1000,
//     };
//
//     if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout,
//                    sizeof(timeout)) == -1) {
//         cout << "error on setsockopt for timeout\n";
//         exit(1);
//     }
//     int mode;
//
//     if (strcmp(argv[1], "--server") == 0) {
//         mode = 0;
//     } else if (strcmp(argv[1], "--client") == 0) {
//         mode = 1;
//     } else {
//         cout << "unrecognized option";
//         return 0;
//     }
//
//     // Teste de envio alternando entre cliente e servidor
//     for (int i = 0; i < 6; i++)
//     {
//         if (mode == 0) {
//             cerr << "\n\n\nI am Server Now\n";
//             KermitPacket message;
//             const char* data = messages[i];
//             message.send(socket, PacketType::data, data, strlen(data));
//             message.confirmSend(socket);
//             cerr << "message: " << i << "\n";
//             mode = 1;
//         }
//
//         else if (mode == 1) {
//             cerr << "\n\n\nI am Client Now\n";
//             std::vector<char> buffer;
//             KermitPacket aux;
//             aux.receive(socket, &buffer);
//             cerr << "Received end transmission\n";
//             cerr << "\tMessage " << i << " Obtained: ";
//             cerr << FONT_MAGENTA;
//             for (char a : buffer)
//                 cerr << a;
//             cerr << FONT_NORMAL;
//             cerr << "\n";
//             mode = 0;
//         }
//     }
// }

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
