#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <bitset>
#include <ctime>
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

#define CRC_XOR_BITS 0b11010101

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
    //unsigned char crc;  // TODO: remove this field later
    // crc comentado para permitir funcionamento do calc crc
    // outras funcoes ajustadas considerando remocao (acho q peguei todas)
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
            sizeof(this->header) + this->header.size + 1;

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

        if (ret < (int)sizeof(this->header) + 1) {
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
    MessageError sendAndWait(int socket, MessageType type, int sequence,
                             const char* data, unsigned int data_size) {
        *this = (KermitMessage){
            .header =
                {
                    .init_marker = KERMIT_INIT_MARKER,
                    .size = (unsigned char)data_size,
                    .sequence =
                        (unsigned char)sequence,  // TODO: handle this later
                    .type = type,
                },
            //.crc = 0,
            .data = {0},
        };

        MessageError ret = this->writeData(data, data_size);
        if (ret != no_error) {
            return ret;
        }
        ret = this->calculateCRC(0, &this.data[data_size]);
        if (ret != no_error) {
            return ret;
        }

        while (true) {
            switch (this->sendMessage(socket)) {
                case MessageError::send_error:
                    cerr << "error when sending message\n";
                    continue;

                case MessageError::no_error:
                    break;
            }
            time_t timestamp = time(NULL);

            // - if we receive a timeout, then there are no messages from the
            // socket (we try to send a message again;
            // int counter = 0;
            while (true) {
                cerr << "trying to receive message\n";
                ret = this->receiveMessage(socket);
                if (ret == recv_timeout) {
                    cerr << "timed out on recv, trying to send message again\n";
                    this->header.type = error; //? Nao entendi pq mudamos o tipo para error - ULISSES
                    break;

                } else if (ret == no_error) {
                    cerr << "received a kermit message\n";
                    break;

                } else {
                    // if we don't receive a valid message in 2 seconds, then we
                    // send send again
                    if (difftime(time(NULL), timestamp) > 8) {
                        cerr << "timed out on receiving kermit messages, "
                                "trying to send message again\n";
                        this->header.type = error; //? Nao entendi pq mudamos o tipo para error - ULISSES
                        break;
                    }
                }
            }

            if (this->header.type == ack) {
                cerr << FONT_GREEN "recieved ACK\n" FONT_NORMAL;
                return no_error;

            } else if (this->header.type == nack) {
                cerr << FONT_RED "received NACK\n" FONT_NORMAL;
            }
        }

        return no_error;
    }

    // requires message to be fully written excluding CRC
    MessageError calculateCRC(unsigned char is_check, unsigned char* crc_return) {
        if (!crc_return)
            return null_pointer;

        unsigned long message_size =
            sizeof(this->header) + this->header.size;

        unsigned char aux_buffer[message_size + 1];
        // If we want to generate a crc, last 8 bits are 0
        if (is_check == 0)
        {
            memcpy(aux_buffer, this, message_size);
            aux_buffer[message_size] = 0;
        }
        // If we want to check a crc, message has crc written on its last 
        // 8 bits (not accounted in message_size)
        else
            memcpy(aux_buffer, this, message_size+1);

        // CRC = first 8 bits of the message (init marker)
        unsigned char crc = aux_buffer[0];

        // Loop for remaining (n-1)*8 bits 
        // doing XoR when 1st bit is == 1
        for (unsigned int  i = 1; i <= message_size; i++) {
            unsigned char current_bit = 0b10000000;
            for (unsigned int j = 8; j > 0; j--) {
                // If first bit == 1
                unsigned char bit_check = crc & 0b10000000;
                crc = crc << 1;
                // Adds jth bit of current byte to crc's LSB
                crc += (aux_buffer[i] & current_bit) >> j-1;

                // If first bit == 1 do XOR with generator
                if (bit_check) {
                    crc = crc ^ CRC_XOR_BITS;
                }
                
                current_bit = current_bit >> 1;
            }
        }

        *crc_return = crc;

        return no_error;
    }

    void printHeader() {
        cerr << "init_marker: " << std::bitset<8>(this->header.init_marker)
             << "\n";
        cerr << "size: " << (int)this->header.size << "\n";
        cerr << "sequence: " << (int)this->header.sequence << "\n";
        cerr << "type: " << (int)this->header.type << "\n";
        //cerr << "crc: " << (int)this->crc << "\n";
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
    // using MessageError = KermitMessage::MessageError;

    unsigned int count = 0;
    while (true) {
        const char* data = messages[count % 5];
        KermitMessage message;
        message.header.sequence = count;
        message.sendAndWait(socket, MessageType::data, count, data,
                            strlen(data));
        // KermitMessage message = {
        //     .header =
        //         {
        //             .init_marker = KERMIT_INIT_MARKER,
        //             .size = (unsigned char)(strlen(data)),
        //             .sequence = (unsigned char)count,
        //             .type = MessageType::data,
        //         },
        //     .crc = 0,
        //     .data = {0},
        // };
        //
        // switch (message.writeData(data, strlen(data))) {
        //     case MessageError::null_pointer:
        //         cerr << "Null pointer when writing to data to message\n";
        //         break;
        //
        //     case MessageError::written_data_too_big:
        //         cerr << "Message too big (for now)\n";
        //         break;
        //
        //     default:
        //         break;
        // }
        //
        // // try to send message until receives ACK/NACK
        // while (true) {
        //     switch (message.sendMessage(socket)) {
        //         case MessageError::send_error:
        //             cerr << "error when sending message\n";
        //             continue;
        //
        //         case MessageError::no_error:
        //             break;
        //     }
        //
        //     switch (message.receiveMessage(socket)) {
        //         case MessageError::recv_timeout:
        //             cerr << "error when reading message on " << __func__
        //                  << "()\n";
        //             continue;
        //
        //         case MessageError::message_received_too_small:
        //             cerr << "message too small on" << __func__ << "()\n";
        //             continue;
        //
        //         case MessageError::wrong_init_marker:
        //             cerr << "unrecognized header on " << __func__ << "()\n";
        //             continue;
        //
        //         case MessageError::no_error:
        //             break;
        //
        //         default:
        //             break;
        //     }
        //
        //     if (message.header.type == ack) {
        //         cerr << FONT_GREEN "received ACK\n" FONT_NORMAL;
        //         break;
        //     } else if (message.header.type == nack) {
        //         cerr << FONT_RED "received NACK\n" FONT_NORMAL;
        //     }
        // }

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

    unsigned char crc_check;
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

        // Checking if CRC error
        message.calculateCRC(1, &crc_check);
        
        // Error
        if (crc_check != 0)
            message.header.type = nack;
        // No Error
        else
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
