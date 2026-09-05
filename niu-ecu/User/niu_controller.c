
#include "niu_controller.h"

#include "global_data.h"
#include "rs485.h"

#include <stdio.h>
#include <stdint.h>


int build_frame(uint8_t address, uint8_t operation, uint8_t *payload, uint8_t payload_length, uint8_t *out_buffer, uint8_t out_buffer_length) {
    uint8_t total_length = 4 + 1 + 1 + payload_length + 1 + 1;  // addr + operation + payload_length byte + payload_data + checksum + end
    if (total_length > out_buffer_length) {
        return -1;
    }

    uint8_t idx = 0;

    // address
    out_buffer[idx++] = 0x68;
    out_buffer[idx++] = address;
    out_buffer[idx++] = ~address & 0xFF;
    out_buffer[idx++] = 0x68;

    // operation
    out_buffer[idx++] = operation;

    // payload length
    out_buffer[idx++] = payload_length;

    // payload data
    for (uint8_t i = 0; i < payload_length; i++) {
        out_buffer[idx++] = payload[i] + 0x33;
    }

    // checksum
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < idx; i++) {
        checksum += out_buffer[i];
    }
    out_buffer[idx++] = checksum;

    // last byte
    out_buffer[idx++] = 0x16;

    return idx;
}


int parse_frame(uint8_t *in_buffer, uint8_t in_buffer_length, uint8_t *out_address, uint8_t *out_operation, uint8_t *out_payload_buffer, uint8_t out_payload_buffer_length) {
    
    // find the start of the frame
    uint8_t frame_start = 0;
    for (; frame_start < in_buffer_length; ++frame_start) {
        if (in_buffer[frame_start] == 0x68) {
            break;
        }
    }
    if (frame_start >= in_buffer_length) {
        return 0;
    }

    uint8_t *frame_buffer = &in_buffer[frame_start];
    uint8_t raw_frame_length = in_buffer_length - frame_start;

    // check length
    uint8_t payload_length_idx = 5;
    if (payload_length_idx >= raw_frame_length) {
        return 0;
    }
    uint8_t payload_length = frame_buffer[payload_length_idx];
    uint8_t frame_length = 4 + 1 + 1 + payload_length + 1 + 1;  // addr + operation + payload_length byte + payload_data + checksum + end
    if (frame_length > raw_frame_length) {
        return 0;
    }

    // check checksum
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < frame_length - 2; ++i) {
        checksum += frame_buffer[i];
    }
    if (checksum != frame_buffer[frame_length - 2]) {
        return 0;
    }

    // check last byte
    if (frame_buffer[frame_length - 1] != 0x16) {
        return 0;
    }

    // check address
    uint8_t first_0x68 = frame_buffer[0];
    uint8_t address = frame_buffer[1];
    uint8_t address_inv = frame_buffer[2];
    uint8_t second_0x68 = frame_buffer[3];
    if (first_0x68 != 0x68 || second_0x68 != 0x68 || address_inv != (~address & 0xFF)) {
        return 0;
    }

    // check output buffer length
    if (payload_length > out_payload_buffer_length) {
        return 0;
    }

    // extract data
    *out_address = address;
    *out_operation = frame_buffer[4];
    uint8_t payload_data_idx = 6;
    for (uint8_t i = 0; i < payload_length; ++i) {
        out_payload_buffer[i] = frame_buffer[payload_data_idx + i] - 0x33;
    }

    return payload_length;
}

int build_frame_with_preamble(uint8_t address, uint8_t operation, uint8_t *payload, uint8_t payload_length, uint8_t *out_buffer, uint8_t out_buffer_length) {
    uint8_t preamble_length = 4;
    if (out_buffer_length < preamble_length) {
        return -2;
    }
    int result = build_frame(address, operation, payload, payload_length, out_buffer + preamble_length, out_buffer_length - preamble_length);
    if (result < 0) {
        return result;
    }
    out_buffer[0] = 0xfe;
    out_buffer[1] = 0xfe;
    out_buffer[2] = 0xfe;
    out_buffer[3] = 0xfe;
    return result + preamble_length;
}


int transact_frame(uint8_t address, uint8_t operation, uint8_t *payload, uint8_t payload_length, uint8_t *out_payload_buffer, uint8_t out_payload_buffer_length) {
    int frame_size = build_frame_with_preamble(
        address, operation, payload, payload_length, g_uart_send_buffer, sizeof(g_uart_send_buffer)
    );
    if (frame_size <= 0) {
        return -1;  // build frame failed
    }

    rs485_enable_send();
    uart_send_frame(g_uart_send_buffer, frame_size);

    rs485_enable_receive();
    int receive_length = uart_receive_frame_timeout(
        g_uart_receive_buffer, sizeof(g_uart_receive_buffer), RECEIVE_FIRST_TIMEOUT_MS, RECEIVE_IDLE_TIMEOUT_MS
    );

    if (receive_length <= 0) {
        return -2;  // receive failed
    }

    uint8_t receive_address = 0;
    uint8_t receive_operation = 0;
    int parse_result = parse_frame(
        g_uart_receive_buffer, receive_length, &receive_address, &receive_operation, out_payload_buffer, out_payload_buffer_length
    );
    if (parse_result <= 0) {
        return -3;  // parse failed
    }
    if (receive_address != address) {
        return -4;  // device mismatch
    }
    if (receive_operation != (operation | 0x80)) {
        return -5;  // operation mismatch
    }

    return parse_result;  // success, parse_result is out_payload data length
}

uint16_t reflect_bits_16(uint16_t value) {
    uint16_t result = 0;
    for (int i = 0; i < 16; ++i) {
        result <<= 1;
        result |= (value & 1);
        value >>= 1;
    }
    return result;
}

uint16_t calc_serial_number_crc16(uint8_t *data, uint8_t length) {
    uint16_t crc = 0xFFFF;

    for (uint8_t i = 0; i < length; ++i) {
        crc ^= data[i];

        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                // 0x7085 bit wise flip is 0xA10E: reflect_bits_16(0x7085) == 0xA10E
                crc = (crc >> 1) ^ 0xA10E;
            }
            else {
                crc >>= 1;
            }
        }
    }
    return crc;
}
