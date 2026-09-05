
#include "niu-packet.h"
#include <stdbool.h>


typedef enum DetectResult {
    DETECT_RESULT_ERROR,
    DETECT_RESULT_INCOMPLETE,
    DETECT_RESULT_COMPLETE
} DetectResult;


static DetectResult _packet_try_parse_packet(PacketDetector *detector, Packet *out_packet);


void packet_init(Packet *packet, uint8_t *data, uint8_t max_size) {
    packet->address = 0;
    packet->operation = 0;
    packet->payload_data = data;
    packet->payload_count = 0;
    packet->payload_max_count = max_size;
}


uint8_t packet_calc_size(Packet *packet) {
    uint8_t total_size = 0;
    total_size += 4;  // head
    total_size += 1;  // operation
    total_size += 1;  // count
    total_size += packet->payload_count;  // payloa
    total_size += 1;  // checksum
    total_size += 1;  // end
    return total_size;
}


uint8_t packet_format_raw(Packet *packet, uint8_t *buffer, uint8_t buffer_size) {
    uint8_t index = 0;
    // head
    buffer[index] = 0x68;
    index += 1;
    buffer[index] = packet->address;
    index += 1;
    buffer[index] = ~packet->address;
    index += 1;
    buffer[index] = 0x68;
    index += 1;
    // operation
    buffer[index] = packet->operation;
    index += 1;
    // count
    buffer[index] = packet->payload_count;
    index += 1;
    // payload_raw
    for (uint8_t i = 0; i < packet->payload_count; ++i) {
        uint8_t b = packet->payload_data[i];
        buffer[index] = b + 0x33;
        index += 1;
    }
    // checksum
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < index; ++i) {
        uint8_t b = buffer[i];
        checksum += b;
    }
    buffer[index] = checksum;
    index += 1;
    // end
    buffer[index] = 0x16;
    index += 1;
    uint8_t total_size = index;
    assert(packet_calc_size(packet) == total_size);
    assert(total_size <= buffer_size);
    return total_size;
}


void packet_detect_init(PacketDetector *detector) {
    detector->state = DETECT_EMPTY;
    detector->head_index = 0;
    detector->body_index = 0;
}


uint8_t packet_detect_push(PacketDetector *detector, uint8_t *data, uint8_t data_size, Packet *out_packet) {
    // max 1 packet count be detected
    uint8_t have_packet = 0;
    for (uint8_t i = 0; i < data_size; ++i) {
        uint8_t b = data[i];
        if (detector->state == DETECT_EMPTY) {
            if (b == 0x68) {
                detector->state = DETECT_HEAD;
                detector->head_index = 0;
                detector->head_data[detector->head_index] = b;
                ++detector->head_index;
            }
        }
        else if (detector->state == DETECT_HEAD) {
            detector->head_data[detector->head_index] = b;
            ++detector->head_index;
            if (detector->head_index >= 4) {
                uint8_t begin = detector->head_data[0];
                uint8_t address = detector->head_data[1];
                uint8_t inv_address = detector->head_data[2];
                uint8_t end = detector->head_data[3];
                if (address ^ inv_address == 0xff && begin == 0x68 && end == 0x68) {
                    detector->state = DETECT_BODY;
                    detector->body_index = 0;
                }
                else {
                    detector->state = DETECT_EMPTY;
                }
            }
        }
        else if (detector->state == DETECT_BODY) {
            detector->body_data[detector->body_index] = b;
            ++detector->body_index;
            if (b == 0x16) {
                DetectResult detect_result = _packet_try_parse_packet(detector, out_packet);
                if (detect_result == DETECT_RESULT_COMPLETE || detect_result == DETECT_RESULT_ERROR) {
                    detector->state = DETECT_EMPTY;
                }
                if (detect_result == DETECT_RESULT_COMPLETE) {
                    if (have_packet) {
                        // more than one packet detect in one push
                        assert(0);
                    }
                    have_packet = 1;
                }
            }
        }
    }
    return have_packet;
}


uint8_t sum_bytes(uint8_t *data, uint8_t len) {
    uint8_t res = 0;
    for (uint8_t i = 0; i < len; ++i) {
        res += data[i];
    }
    return res;
}


static DetectResult _packet_try_parse_packet(PacketDetector *detector, Packet *out_packet) {
    // op count payload checksum 0x16
    uint8_t *data = detector->body_data;
    uint8_t data_len = detector->body_index;
    uint8_t index = 0;
    uint8_t op;
    uint8_t count;
    uint8_t checksum;
    uint8_t calculated_checksum;
    uint8_t last;
    uint8_t *payload_raw_data;
    uint8_t address;

    if (index < data_len) {
        op = data[index];
        ++index;
    }
    else {
        return DETECT_RESULT_INCOMPLETE;
    }
    if (index < data_len) {
        count = data[index];
        ++index;
    }
    else {
        return DETECT_RESULT_INCOMPLETE;
    }
    if (index + count < data_len) {
        payload_raw_data = data + index;
        index += count;
    }
    else {
        return DETECT_RESULT_INCOMPLETE;
    }
    if (index < data_len) {
        checksum = data[index];
        ++index;
    }
    else {
        return DETECT_RESULT_INCOMPLETE;
    }
    if (index < data_len) {
        last = data[index];
        index += 1;
    }
    else {
        return DETECT_RESULT_INCOMPLETE;
    }
    if (last != 0x16) {
        // last byte error
        return DETECT_RESULT_ERROR;
    }
    calculated_checksum = sum_bytes(detector->head_data, 4);
    calculated_checksum += sum_bytes(detector->body_data, index - 2);
    if (checksum != calculated_checksum) {
        // checksum error
        return DETECT_RESULT_ERROR;
    }

    address = detector->head_data[1];
    out_packet->address = address;
    out_packet->operation = op;
    for (uint8_t i = 0; i < count; ++i) {
        uint8_t b = payload_raw_data[i];
        out_packet->payload_data[i] = b - 0x33;
    }
    out_packet->payload_count = count;
    return DETECT_RESULT_COMPLETE;
}
