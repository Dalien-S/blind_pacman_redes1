#include "kermit.hpp"

#include <string.h>

#include <bitset>
#include <ctime>

#include "logging.hpp"
// #include <cstdlib>

// #include "macros.hpp"


Logger kermit_logger;

void setKermitLogger(const char* file_path) {
    kermit_logger = Logger::initLogger(file_path);
}

void unsetKermitLogger() {
    Logger::terminateLogger(&kermit_logger);
}

KermitPacket::KermitPacket() {}
KermitPacket::KermitPacket(PacketType type, unsigned char sequence) {
    this->header = {
        .init_marker = KERMIT_INIT_MARKER,
        .size = 0,
        .sequence = sequence,
        .type = type,
    };
    memset(this->data, 0, BUFFER_SIZE + 1);
}

// TODO: this function needs to calculate(and set) the CRC too
PacketError KermitPacket::writeData(const char* data, int data_size) {
    if (!data) {
        return null_pointer;
    }

    if (data_size > 32 /*max value for 5b number is 32*/) {
        return written_data_too_big;
    }

    memcpy(this->data, data, data_size);
    this->header.size = data_size;

    return no_error;
}

int KermitPacket::sendPacket(int socket) {
    unsigned long message_struct_size =
        sizeof(this->header) + this->header.size + 1;

    // just so there's no problem with the size of the message
    char frame[2 * sizeof(*this)];
    char struct_copy[message_struct_size];
    memset(frame, 0, message_struct_size + MINIMUM_PACKET_SIZE);

    memcpy(struct_copy, this, message_struct_size);
    unsigned long written_bytes = 0;
    for (unsigned long i = 0; i < message_struct_size; i++) {
        frame[written_bytes] = struct_copy[i];
        if (struct_copy[i] == (char)0x88 || struct_copy[i] == (char)0x81) {
            written_bytes++;
            frame[written_bytes] = (char)0xff;
        }
        written_bytes++;
    }
    if (written_bytes < MINIMUM_PACKET_SIZE) {
        written_bytes = MINIMUM_PACKET_SIZE;
    }

    if (::send(socket, (const void*)frame, written_bytes, 0) == -1) {
        return send_error;
    }

    // kermit_logger.printColor(color::red, "printing data\n");
    kermit_logger.print("data sent (escaped): (");
    for (unsigned int i = 0; i < written_bytes; i++) {
        kermit_logger.printColor(color::red, "%02x", frame[i]);
    }
    kermit_logger.print(")\n");

    return no_error;
}

PacketError KermitPacket::receivePacket(int socket) {
    // int max_received_size = 2 * sizeof(*this);
    char buffer[2 * sizeof(*this)];
    int ret = recv(socket, buffer, 2 * sizeof(*this), 0);

    if (ret == -1) {
        if (errno == ETIMEDOUT) {
            return recv_timeout;
        }
        return recv_other_error;
    }

    int written_bytes = 0;
    for (int i = 0; i < ret && written_bytes < (int)sizeof(*this); i++) {
        ((unsigned char*)this)[written_bytes] = buffer[i];
        if (buffer[i] == (char)0x88 || buffer[i] == (char)0x81) {
            i++;
        }
        written_bytes++;
    }

    if (written_bytes < (int)sizeof(this->header) + 1) {
        return message_received_too_small;
    }

    if (this->header.init_marker != KERMIT_INIT_MARKER) {
        return wrong_init_marker;
    }


    if (checkCRC() == false) {
        return wrong_crc;
    }

    kermit_logger.print("data received (escaped): (");
    for (int i = 0; i < ret; i++) {
        kermit_logger.printColor(color::red, "%02x", buffer[i]);
    }
    kermit_logger.print(")\n");
    // this->printData();

    return no_error;
}

// - sends a message and expects an ACK in return from the socket;
// - if there's no message in return or if it receives NACK, then try
// sending the same message again
//
// - if the message type doesn't involve data (eg. ack/nack), then the
// parameter data and data size are ignored
PacketError KermitPacket::send(int socket, PacketType type, const char* data,
                               unsigned int data_size) {
    kermit_logger.print((char*)"entered send()\n");
    // cerr << "entered send()\n";

    int sequence = 0;

    KermitPacket init = KermitPacket(initialize, 0);
    init.setCRC();
    while (true) {
        KermitPacket response;
        int ret = init.sendPacket(socket);
        if (ret == no_error) {
            ret = response.receivePacket(socket);
            if (ret == no_error) {
                if (response.header.type == ack) {
                    kermit_logger.print(
                        "received ack for the starting packet\n");
                    break;
                } else {
                    kermit_logger.print(
                        "didn't receive ack for the starting packet\n");
                }
            } else {
                kermit_logger.print(
                    "didn't receive any response, trying again...\n");
            }
        }
    }

    if (data_size == 0) {
        KermitPacket packet = KermitPacket(type, 0);
        packet.setCRC();
        while (true) {
            KermitPacket response;
            int ret = packet.sendPacket(socket);
            if (ret == no_error) {
                ret = response.receivePacket(socket);
                if (ret == no_error) {
                    if (response.header.type == ack) {
                        break;
                    } else {
                        kermit_logger.print(
                            "didn't receive ACK as response, trying "
                            "again...\n");
                    }
                } else {
                    kermit_logger.print(
                        "didn't receive any response, trying again...\n");
                }
            }
        }

        return no_error;
    }

    unsigned int offset = 0;  // position on the data buffer in bytes

    while (offset < data_size) {
        unsigned int distance_to_end =
            std::abs((long int)data_size - (long int)offset);

        int size = BUFFER_SIZE;
        if (distance_to_end < BUFFER_SIZE) {
            size = distance_to_end;
        }

        // cerr << "data size: " << size << " ";
        // cerr.write(data + offset, size + 1);
        // cerr << "\n";

        KermitPacket packet = KermitPacket(type, sequence);
        PacketError ret = packet.writeData(data + offset, size);
        packet.setCRC();

        if (ret != no_error) {
            kermit_logger.print((char*)"errror when writing data to buffer\n");
            // cerr << "error when writing data to buffer\n";
            return ret;
        }

        while (true) {
            int ret = packet.sendPacket(socket);
            if (ret == send_error) {
                kermit_logger.print((char*)"error when sending message\n");
                // cerr << "error when sending message\n";
                continue;
            }
            time_t timestamp = time(NULL);

            // - if we receive a timeout, then there are no messages from the
            // socket (we try to send a message again;
            KermitPacket response;
            while (true) {
                ret = response.receivePacket(socket);
                if (ret == recv_timeout) {
                    kermit_logger.print((
                        char*)"timed out on recv, trying to send message "
                              "again\n");
                    // cerr << "timed out on recv, trying to send message
                    // again\n";
                    break;
                } else if (ret == no_error) {
                    kermit_logger.print((char*)"received a kermit message\n");
                    // cerr << "received a kermit message\n";
                    break;

                } else {
                    // if we don't receive a valid message in 2 seconds, then we
                    // send again
                    if (difftime(time(NULL), timestamp) > 8) {
                        kermit_logger.print(
                            "timed out on receiving kermit messages, "
                            "trying to send message again\n");
                        // cerr << "timed out on receiving kermit messages, "
                        //         "trying to send message again\n";
                        break;
                    }
                }
            }

            if (ret == no_error) {
                if (response.header.type == ack) {
                    kermit_logger.printColor(color::green, "received ACK\n");
                    sequence = (sequence + 1) %
                               64;  // 8 because sequence field has 6 bits
                    offset += size;
                } else if (response.header.type == nack) {
                    kermit_logger.printColor(color::red, "received NACK");
                    // cerr << color::red << "received NACK\n" << color::normal;
                }
                break;
            }
        }
    }
    kermit_logger.print("entire message sent, exiting send()\n");
    // cerr << "entire message sent, exiting send()\n";

    // KermitPacket end = KermitPacket(end_transmission, 0);
    // end.setCRC();
    // while (true) {
    //     if (end.sendPacket(socket) == no_error) {
    //         break;
    //     }
    // }

    return no_error;
}

PacketError KermitPacket::confirmSend(int socket) {
    kermit_logger.print("entering confirmSend()\n");
    // cerr << "entering confirmSend()\n";
    KermitPacket end = KermitPacket(end_transmission, 0);
    end.setCRC();
    while (true) {
        kermit_logger.print("sending end_transmission\n");
        // cerr << "sending end_transmission\n";
        if (end.sendPacket(socket) == no_error) {
            break;
        }
    }

    while (true) {
        KermitPacket response;

        PacketError ret = response.receivePacket(socket);

        if (ret == PacketError::no_error) {
            if (response.header.type == ack) {
                break;
            } else if (response.header.type == nack) {
                KermitPacket response_ack = KermitPacket(ack, 0);
                response_ack.setCRC();
                while (true) {
                    kermit_logger.print("sending end_transmission\n");
                    // cerr << "sending end_transmission\n";
                    if (response_ack.sendPacket(socket) != no_error) break;
                }
                break;
            }
        }
    }
    kermit_logger.print(
        "successfully sent end_transmission, exiting confirmSend()\n");
    // cerr << "successfully sent end_transmission exiting confirmSend()\n";

    return no_error;
}

PacketType KermitPacket::receive(int socket, std::vector<char>* buffer) {
    // TODO: flush buffer
    buffer->clear();

    kermit_logger.print("entering receive()\n");
    // cerr << "entering receive()\n";
    KermitPacket packet;

    // Resposta pronta ack
    KermitPacket response_ack = KermitPacket(ack, 0);
    response_ack.setCRC();

    // resposta pronta nack
    KermitPacket response_nack = KermitPacket(nack, 0);
    response_nack.setCRC();

    int ret;
    int sequence = 0;

    PacketType message_type;

    bool received_initialize = false;
    while (true) {
        ret = packet.receivePacket(socket);

        if (ret == PacketError::no_error) {
            if (packet.header.type == initialize) {
                kermit_logger.printColor(color::cyan, "received initialize\n");
                // cerr << color::cyan << "received initialize\n" <<
                // color::normal;
                response_ack.sendPacket(socket);
                received_initialize = true;
            } else if (received_initialize) {
                break;
            }
        } else if (ret == wrong_crc) {
            response_nack.sendPacket(socket);
        } else {
            continue;
        }
    }

    kermit_logger.printColor(color::cyan, "Sequence (%d) received", sequence);
    // cerr << "Sequence " << sequence << " Received, ";
    kermit_logger.print("Inserting data to buffer\n");
    // cerr << "Inserting Data to Buffer\n";
    buffer->insert(buffer->end(), packet.data,
                   packet.data + packet.header.size);
    response_ack.sendPacket(socket);
    message_type = packet.header.type;

    while (true) {
        int ret = packet.receivePacket(socket);

        if (ret == no_error) {
            // Received end transmission
            if (packet.header.type == end_transmission) {
                kermit_logger.printColor(color::blue, "END TRANSMISSION\n");
                // cerr << color::blue << "END TRANSMISSION\n" << color::normal;
                response_ack.sendPacket(socket);
                break;
            }
            // Wrong Type of Message
            else if (packet.header.type != message_type) {
                kermit_logger.printColor(color::blue, "Wrong type\n");
                // cerr << color::blue << "Wrong Type\n" << color::normal;
                response_nack.sendPacket(socket);
            }
            // Next Sequential Message
            else if (packet.header.sequence == (sequence + 1) % 64) {
                sequence = (sequence + 1) % 64;
                kermit_logger.printColor(
                    color::blue, "RECEIVED NEXT SEQUENCE (%d)\n", sequence);
                // cerr << color::blue << "RECEIVED NEXT SEQUENCE\n"
                //      << color::normal;
                // cerr << "Sequence " << sequence << " Received, ";
                // cerr << "Inserting Data to Buffer\n";
                kermit_logger.print("Inserting data to buffer\n");
                buffer->insert(buffer->end(), packet.data,
                               packet.data + packet.header.size);
                response_ack.sendPacket(socket);
            }
            // Wrong Sequential Message
            else if (packet.header.sequence < sequence) {
                kermit_logger.printColor(
                    color::blue,
                    "WRONG SEQUENCE: expected (%d), but got (%d)\n", sequence,
                    (unsigned char)packet.header.sequence);
                // cerr << color::blue << "WRONG SEQUENCE: expected (" <<
                // sequence
                //      << ") but got (" << (unsigned
                //      char)packet.header.sequence
                //      << ")\n"
                //      << color::normal;
                response_nack.sendPacket(socket);
            }
            // Same message
            else {
                kermit_logger.printColor(color::blue,
                                         "RECEIVED SAME MESSAGE\n");
                kermit_logger.print("Sequence (%d) received\n", sequence);
                // cerr << color::blue << "RECEIVED SAME MESSAGE\n"
                //      << color::normal;
                // cerr << "Sequence " << sequence << " Received, ";
                response_ack.sendPacket(socket);
            }

        }
        // Wrong CRC Invalid Message
        else if (ret == wrong_crc) {
            kermit_logger.print("wrong crc\n");
            // cerr << "Wrong CRC\n";
            response_nack.sendPacket(socket);
        } else
            continue;
    }

    kermit_logger.print("exiting receive()\n");
    // cerr << "exiting receive()\n";
    return message_type;
}

// requires message to be fully written excluding CRC
PacketError KermitPacket::calculateCRC(bool is_check, char* crc_return) {
    if (!crc_return) return null_pointer;

    unsigned long message_size = sizeof(this->header) + this->header.size;

    unsigned char aux_buffer[message_size + 1];
    // If we want to generate a crc, last 8 bits are 0
    if (!is_check) {
        memcpy(aux_buffer, this, message_size);
        aux_buffer[message_size] = 0;
    }
    // If we want to check a crc, message has crc written on its last
    // 8 bits (not accounted in message_size)
    else
        memcpy(aux_buffer, this, message_size + 1);

    // CRC = first 8 bits of the message (init marker)
    unsigned char crc = aux_buffer[0];

    // Loop for remaining (n-1)*8 bits
    // doing XoR when 1st bit is == 1
    for (unsigned int i = 1; i <= message_size; i++) {
        unsigned char current_bit = 0b10000000;
        for (unsigned int j = 8; j > 0; j--) {
            // If first bit == 1
            unsigned char bit_check = crc & 0b10000000;
            crc = crc << 1;
            // Adds jth bit of current byte to crc's LSB
            crc += (aux_buffer[i] & current_bit) >> (j - 1);

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

void KermitPacket::setCRC() {
    calculateCRC(false, &this->data[this->header.size]);
}

bool KermitPacket::checkCRC() {
    char crc;
    calculateCRC(true, &crc);

    if (crc == 0) return true;

    return false;
}

void KermitPacket::printHeader() {
    kermit_logger.print("init_marker: %8b\n", this->header.init_marker);
    // cerr << "init_marker: " << std::bitset<8>(this->header.init_marker) << "\n";
    kermit_logger.print("size: %d\n", this->header.size);
    // cerr << "size: " << (int)this->header.size << "\n";
    kermit_logger.print("sequence: %d\n", this->header.sequence);
    // cerr << "sequence: " << (int)this->header.sequence << "\n";
    kermit_logger.print("type: %d\n", this->header.type);
    // cerr << "type: " << (int)this->header.type << "\n";
}

void KermitPacket::printData() {
    // cerr << std::bitset<BUFFER_SIZE>(this->data) << "\n";
    // cerr << "(" << color::red;
    // cerr << "(";
    kermit_logger.print("(");
    for (int i = 0; i < this->header.size; i++) {
        kermit_logger.printColor(color::red, "%02x",
                                 (char)this->data[i]);
    }
    kermit_logger.print(")\n");
    // cerr << ")\n";
}
