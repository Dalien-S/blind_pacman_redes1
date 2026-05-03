#include "kermit.hpp"

#include <string.h>

#include <bitset>
#include <ctime>
// #include <cstdlib>

#include "macros.hpp"

// TODO: this function needs to calculate(and set) the CRC too
PacketError KermitPacket::writeData(const char* data, int data_size) {
    if (!data) {
        return null_pointer;
    }

    if (data_size > 32 /*max value for 5b number is 32*/) {
        return written_data_too_big;
    }

    memcpy(this->data, data, data_size);

    return no_error;
}

int KermitPacket::sendPacket(int socket) {
    unsigned long message_struct_size =
        sizeof(this->header) + this->header.size + 1;

    // just so there's no problem with the size of the message
    char frame[message_struct_size + MINIMUM_PACKET_SIZE];

    memcpy(frame, this, message_struct_size);
    if (message_struct_size < MINIMUM_PACKET_SIZE) {
        message_struct_size += MINIMUM_PACKET_SIZE - message_struct_size;
    }

    if (::send(socket, (const void*)this, message_struct_size, 0) == -1) {
        return send_error;
    }

    return no_error;
}

PacketError KermitPacket::receivePacket(int socket) {
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

    if (checkCRC() == false){
        return wrong_crc;}

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
    unsigned int offset = 0;  // position on the data buffer in bytes
    int sequence = 0;

    while (offset < data_size) {
        unsigned int distance_to_end =
            std::abs((long int)data_size - (long int)offset);

        int size = BUFFER_SIZE;
        if (distance_to_end < BUFFER_SIZE) {
            size = distance_to_end;
        }

        cerr << "data size: " << size << " ";
        cerr.write(data + offset, size);
        cerr << "\n";

        KermitPacket packet = (KermitPacket){
            .header =
                {
                    .init_marker = KERMIT_INIT_MARKER,
                    .size = (unsigned char)size,
                    .sequence = (unsigned char)sequence,  // TODO: handle this later
                    .type = type,
                },
            .data = {0},
        };

        PacketError ret = packet.writeData(data + offset, size);

        if (ret != no_error) {
            cerr << "error when writing data to buffer\n";
            return ret;
        }

        packet.setCRC();

        while (true) {
            int ret = packet.sendPacket(socket);
            if (ret == send_error) {
                cerr << "error when sending message\n";
                continue;
            }
            // No need to break for no error go to receive ack
            /*else if (ret == no_error) {
                break;
            }*/

            time_t timestamp = time(NULL);

            // - if we receive a timeout, then there are no messages from the
            // socket (we try to send a message again;
            KermitPacket response;
            while (true) {
                ret = response.receivePacket(socket);
                if (ret == recv_timeout) {
                    cerr << "timed out on recv, trying to send message again\n";
                    break;
                } else if (ret == no_error) {
                    cerr << "received a kermit message\n";
                    break;

                } else {
                    // if we don't receive a valid message in 2 seconds, then we
                    // send again
                    if (difftime(time(NULL), timestamp) > 8) {
                        cerr << "timed out on receiving kermit messages, "
                                "trying to send message again\n";
                        break;
                    }
                }
            }

            if (ret == no_error)
            {
                if (response.header.type == ack) {
                    cerr << FONT_GREEN "recieved ACK\n" FONT_NORMAL;
                    break;

                } else if (response.header.type == nack) {
                    cerr << FONT_RED "received NACK\n" FONT_NORMAL;
                }
            }
        }

        sequence = (sequence + 1) % 8; // 8 because sequence field has 3 bits
        offset += size;
    }

    return no_error;
}

PacketError KermitPacket::confirmSend(int socket) {
    /*
    KermitPacket end_message = {
        .header =
            {
                .init_marker = KERMIT_INIT_MARKER,
                .size = 5,
                .sequence = 0,
                .type = PacketType::finalize,
            },
        .data = {0},
    };
    end_message.setCRC();
    */
    while (true) {
        KermitPacket response;

        PacketError ret = response.receivePacket(socket);

        if (ret == PacketError::no_error) {
            if (response.header.type == ack) {
                break;
            }
            continue;
        }
    }

    return no_error;
}

PacketType KermitPacket::receive(int socket, std::vector<char> *buffer) { 
    KermitPacket packet;

    // Resposta pronta ack
    KermitPacket response_ack = (KermitPacket){
        .header =
            {
                .init_marker = KERMIT_INIT_MARKER,
                .size = 0,
                .sequence = 0,         
                .type = ack,               
            },
        
        .data = {0},
    };
    response_ack.calculateCRC(false, response_ack.data);

    // resposta pronta nack
    KermitPacket response_nack = (KermitPacket){
        .header =
            {
                .init_marker = KERMIT_INIT_MARKER,
                .size = 0,
                .sequence = 0,         
                .type = nack,               
            },
        
        .data = {0},
    };
    response_nack.calculateCRC(false, response_nack.data);

    int ret;
    int sequence = 0;

    PacketType message_type;

    while (true) {
        ret = packet.receivePacket(socket);

        if (ret == PacketError::no_error) {
            if (packet.header.type == initialize)
                response_ack.sendPacket(socket);
            else
                break;
        }
        else if (ret == wrong_crc)
            response_nack.sendPacket(socket);
        else
            continue;
    }
    
    cerr << "Sequence " << sequence << "Received";
    cerr << "Inserting Data to Buffer\n";
    buffer -> insert(buffer->end(), packet.data, packet.data + packet.header.size);
    response_ack.sendPacket(socket);
    message_type = packet.header.type;

    while (true)
    {
        int ret = packet.receivePacket(socket);

        if (ret == no_error)
        {
            // Received end transmission
            if (packet.header.type == end_transmission) {   
                cerr << "END TRANSMISSION\n";
                break;
            }
            // Wrong Type of Message 
            else if (packet.header.type != message_type) {
                cerr << "Wrong Type\n";
                response_nack.sendPacket(socket);
            } 
            // Next Sequential Message
            else if (packet.header.sequence == sequence + 1) {
                cerr << "Sequence " << sequence << "Received";
                cerr << "Inserting Data to Buffer\n";
                buffer -> insert(buffer->end(), packet.data, packet.data + packet.header.size);
                sequence++;
                response_ack.sendPacket(socket);
            } 
            // Wrong Sequential Message 
            else if (packet.header.sequence < sequence) {
                cerr << "Wrong Sequence\n";
                response_nack.sendPacket(socket);
            } 
            // Same message
            else {
                cerr << "Sequence " << sequence << "Received";
                response_ack.sendPacket(socket);
            }

        } 
        // Wrong CRC Invalid Message
        else if (ret == wrong_crc){
            cerr << "Wrong CRC\n";
            response_nack.sendPacket(socket);
        } else 
            continue;
    }

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
    cerr << "init_marker: " << std::bitset<8>(this->header.init_marker) << "\n";
    cerr << "size: " << (int)this->header.size << "\n";
    cerr << "sequence: " << (int)this->header.sequence << "\n";
    cerr << "type: " << (int)this->header.type << "\n";
    // cerr << "crc: " << (int)this->crc << "\n";
}

void KermitPacket::printData() {
    // cerr << std::bitset<BUFFER_SIZE>(this->data) << "\n";
    cerr << "(" FONT_RED;
    cerr.write(this->data, this->header.size);
    cerr << FONT_NORMAL ")\n";
}
