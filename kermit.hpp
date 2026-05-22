#ifndef KERMIT
#define KERMIT

#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "logging.hpp"

#define KERMIT_INIT_MARKER 0b01111110

#define TIMEOUT_MS 3000  // 3 seconds for timeout

#define BUFFER_SIZE 31  // represented by 5 bits (0-31)
#define MINIMUM_PACKET_SIZE 14

#define CRC_XOR_BITS 0b11010101

using std::cerr;
using std::cout;

// sets the logger for the kermit instance
void setKermitLogger(const char* file_path);
void unsetKermitLogger();

enum PacketType {
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
    finalize = 17,
};
typedef enum PacketType PacketType;

typedef enum {
    null_pointer,
    written_data_too_big,
    message_received_too_small,
    send_error,
    recv_other_error,
    recv_timeout,
    wrong_init_marker,
    no_error,
    wrong_crc,
} PacketError;

// message that follows the kermit protocol
struct KermitPacket {
    struct {
        unsigned char init_marker = KERMIT_INIT_MARKER;
        unsigned char size : 5;  // max size is 32B!!!
        unsigned char sequence : 6;
        PacketType type : 5;
    } header;
    char data[BUFFER_SIZE +
              1];  // data stores the message bytes and the crc right after;
                   // this buffer can store any message size from the protocol

    KermitPacket();
    KermitPacket(PacketType type, unsigned char sequence);

    // writes data to the message
    // TODO: this function needs to calculate the CRC too
    PacketError writeData(const char* data, int data_size);
    int sendPacket(int socket);
    PacketError receivePacket(int socket);

    // - sends a message and expects an ACK in return from the socket;
    // - if there's no message in return or if it receives NACK, then try
    // sending the same message again
    //
    // - if the message type doesn't involve data (eg. ack/nack), then the
    // parameter data and data size are ignored
    PacketError send(int socket, PacketType type, const char* data,
                     unsigned int data_size);
    PacketError confirmSend(int socket);
    PacketType receive(int socket, std::vector<char> *buffer);
    // requires message to be fully written excluding CRC
    PacketError calculateCRC(bool is_check, char* crc_return);
    void setCRC();
    bool checkCRC();
    void printHeader();
    void printData();
};

#endif
