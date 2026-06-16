#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <vector>

#include "kermit.hpp"
#include "raw_sockets.hpp"

#include "server.hpp"
#include "client.hpp"

using std::cerr;
using std::cout;


int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: <program> --client|--server socket [mapfile]\n";
        exit(1);
    }

    int socket = cria_raw_socket((char*)argv[2]);
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
        // If map is defined
        if (argc == 4)
            runServer(socket, argv[3]);
        else
            runServer(socket, "ufpr.csv");
    } else if (strcmp(argv[1], "--client") == 0) {
        runClient(socket);
    } else {
        cout << "unrecognized option";
        return 0;
    }

    return 0;
}
