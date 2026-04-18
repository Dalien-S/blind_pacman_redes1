#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <bitset>
#include <iostream>

#include "raw_sockets.hpp"

#define ERROR -1
#define NOERROR 0

// códigos de cor ANSI (terminal)
#define FONT_NORMAL "\x1B[0m"
#define FONT_BLACK "\x1B[30m"
#define FONT_RED "\x1B[31m"
#define FONT_GREEN "\x1B[32m"
#define FONT_YELLOW "\x1B[33m"
#define FONT_BLUE "\x1B[34m"
#define FONT_MAGENTA "\x1B[35m"
#define FONT_CYAN "\x1B[36m"
#define FONT_WHITE "\x1B[37m"
#define BACK_BLACK "\x1B[40m"
#define BACK_RED "\x1B[41m"
#define BACK_GREEN "\x1B[42m"
#define BACK_YELLOW "\x1B[43m"
#define BACK_BLUE "\x1B[44m"
#define BACK_MAGENTA "\x1B[45m"
#define BACK_CYAN "\x1B[46m"
#define BACK_WHITE "\x1B[47m"

#define KERMIT_INIT_MARKER 0b01111110

#define TIMEOUT_MS 3000  // 3 seconds for timeout

#define BUFFER_SIZE 32  // represented by 5 bits
#define MINIMUM_MESSAGE_SIZE 14

using std::cerr;
using std::cout;

const char* messages[] = {
    "pang", "\tpeng", "\t\tping", "\t\t\tpong", "\t\t\t\tpung",
};

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
};
typedef enum MessageType MessageType;

// message that follows the kermit protocol
struct KermitMessage {
    struct {
        unsigned char init_marker = KERMIT_INIT_MARKER;
        unsigned char size : 5;  // max size is 32B!!!
        unsigned char sequence : 6;
        MessageType type : 5;
    } header;
    unsigned char crc;  // TODO: remove this field later
    char data[BUFFER_SIZE +
              1];  // data stores the message bytes and the crc right after;
                   // this buffer can store any message size from the protocol

    typedef enum {
        null_pointer,
        written_data_too_big,
        message_received_too_small,
        send_error,
        recv_other_error,
        recv_timeout,
        wrong_init_marker,
        no_error,
    } MessageError;

    // writes data to the message
    // TODO: this function needs to calculate the CRC too
    MessageError writeData(const char* data, int data_size) {
        if (!data) {
            return null_pointer;
        }

        if (data_size > 32 /*max value for 5b number is 32*/) {
            return written_data_too_big;
        }

        memcpy(this->data, data, data_size);

        return no_error;
    }

    int sendMessage(int socket) {
        unsigned long message_struct_size =
            sizeof(this->header) + this->header.size + sizeof(this->crc);

        // just so there's no problem with the size of the message
        char frame[message_struct_size + MINIMUM_MESSAGE_SIZE];

        memcpy(frame, this, message_struct_size);
        if (message_struct_size < MINIMUM_MESSAGE_SIZE) {
            message_struct_size += MINIMUM_MESSAGE_SIZE - message_struct_size;
        }

        if (send(socket, (const void*)this, message_struct_size, 0) == -1) {
            return send_error;
        }

        return no_error;
    }

    MessageError receiveMessage(int socket) {
        int ret = recv(socket, this, sizeof(*this), 0);

        if (ret == -1) {
            if (errno == ETIMEDOUT) {
                return recv_timeout;
            }
            return recv_other_error;
        }

        if (ret < (int)sizeof(this->header) + (int)sizeof(this->crc)) {
            return message_received_too_small;
        }

        if (this->header.init_marker != KERMIT_INIT_MARKER) {
            return wrong_init_marker;
        }

        return no_error;
    }

    // - sends a message and expects an ACK in return from the socket;
    // - if there's no message in return or if it receives NACK, then try
    // sending the same message again
    //
    // - if the message type doesn't involve data (eg. ack/nack), then the
    // parameter data and data size are ignored
    MessageError sendAndWait(int socket, MessageType type, const char* data,
                             unsigned int data_size) {
        *this = (KermitMessage){
            .header =
                {
                    .init_marker = KERMIT_INIT_MARKER,
                    .size = (unsigned char)data_size,
                    .sequence = (unsigned char)0,  // TODO: handle this later
                    .type = type,
                },
            .crc = 0,
            .data = {0},
        };

        MessageError ret = this->writeData(data, data_size);
        if (ret != no_error) {
            return ret;
        }

        int counter = 0;
        while (true) {
            switch (this->sendMessage(socket)) {
                case MessageError::send_error:
                    cerr << "error when sending message\n";
                    continue;

                case MessageError::no_error:
                    break;
            }

            // - if we receive a timeout, then there are no messages from the
            // socket (we try to send a message again;
            // - if we receive 15 messages without an ACK response, then we try
            // to send the message again
            int counter = 0;
            do {
                ret = this->receiveMessage(socket);
                if (ret == recv_timeout) {
                    this->header.type = error;
                    break;
                } else {
                    if (++counter > 15) {
                        this->header.type = error;
                        break;
                    }
                }
            } while (ret != no_error);

            if (this->header.type == ack) {
                cerr << FONT_GREEN "recieved ACK\n" FONT_NORMAL;
                return no_error;

            } else if (this->header.type == nack) {
                cerr << FONT_RED "received NACK\n" FONT_NORMAL;
            }
        }

        return no_error;
    }

    void printHeader() {
        cerr << "init_marker: " << std::bitset<8>(this->header.init_marker)
             << "\n";
        cerr << "size: " << (int)this->header.size << "\n";
        cerr << "sequence: " << (int)this->header.sequence << "\n";
        cerr << "type: " << (int)this->header.type << "\n";
        cerr << "crc: " << (int)this->crc << "\n";
    }

    void printData() {
        // cerr << std::bitset<BUFFER_SIZE>(this->data) << "\n";
        cerr << "(" FONT_RED;
        cerr.write(this->data, this->header.size);
        cerr << FONT_NORMAL ")\n";
    }
};
typedef struct MessageHeader MessageHeader;

int runServer(int socket) {
    using MessageError = KermitMessage::MessageError;

    unsigned int count = 0;
    while (true) {
        const char* data = messages[count % 5];
        KermitMessage message = {
            .header =
                {
                    .init_marker = KERMIT_INIT_MARKER,
                    .size = (unsigned char)(strlen(data)),
                    .sequence = (unsigned char)count,
                    .type = MessageType::data,
                },
            .crc = 0,
            .data = {0},
        };

        switch (message.writeData(data, strlen(data))) {
            case MessageError::null_pointer:
                cerr << "Null pointer when writing to data to message\n";
                break;

            case MessageError::written_data_too_big:
                cerr << "Message too big (for now)\n";
                break;

            default:
                break;
        }

        // try to send message until receives ACK/NACK
        while (true) {
            switch (message.sendMessage(socket)) {
                case MessageError::send_error:
                    cerr << "error when sending message\n";
                    continue;

                case MessageError::no_error:
                    break;
            }

            switch (message.receiveMessage(socket)) {
                case MessageError::recv_timeout:
                    cerr << "error when reading message on " << __func__
                         << "()\n";
                    continue;

                case MessageError::message_received_too_small:
                    cerr << "message too small on" << __func__ << "()\n";
                    continue;

                case MessageError::wrong_init_marker:
                    cerr << "unrecognized header on " << __func__ << "()\n";
                    continue;

                case MessageError::no_error:
                    break;

                default:
                    break;
            }

            if (message.header.type == ack) {
                cerr << FONT_GREEN "received ACK\n" FONT_NORMAL;
                break;
            } else if (message.header.type == nack) {
                cerr << FONT_RED "received NACK\n" FONT_NORMAL;
            }
        }

        cerr << "message: " << count << "\n";

        count++;
        if (count % 5 == 0) {
            message.header.init_marker = 8;
            for (int i = 0; i < 15; i++) {
                message.sendMessage(socket);
                cerr << "sent dummy message: " << i << "\n";
            }
            break;
        }
    }

    return 0;
}

int runClient(int socket) {
    using MessageError = KermitMessage::MessageError;

    unsigned char sequence = 0;
    while (true) {
        KermitMessage message;

        switch (message.receiveMessage(socket)) {
            case MessageError::no_error:
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

        if (message.header.sequence == sequence) {
            sequence++;
        } else {
            continue;
        }

        cout << "received message: \n";
        message.printHeader();
        message.printData();

        cout << "sending NACK and then ACK response\n";
        message.header.type = nack;
        message.sendMessage(socket);
        message.header.type = ack;
        switch (message.sendMessage(socket)) {
            case MessageError::send_error:
                cerr << "error on send()\n";
                exit(1);
                break;

            case MessageError::no_error:
                cerr << "no error when sending message\n";
                break;

            default:
                cerr << "AAAAAAAAAAAAAAAAAAAAAAAA\n";
                exit(1);
                break;
        }
    }
}

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
