/*
niu packet frame build and parse
*/

#ifndef NIU_CONTROLLER_H_
#define NIU_CONTROLLER_H_

#include <stdint.h>

#define RECEIVE_FIRST_TIMEOUT_MS 100
#define RECEIVE_IDLE_TIMEOUT_MS 10

int build_frame(uint8_t address, uint8_t operation, uint8_t *payload, uint8_t payload_length, uint8_t *out_buffer, uint8_t out_buffer_length);

int parse_frame(uint8_t *in_buffer, uint8_t in_buffer_length, uint8_t *out_address, uint8_t *out_operation, uint8_t *out_payload_buffer, uint8_t out_payload_buffer_length);

int build_frame_with_preamble(uint8_t address, uint8_t operation, uint8_t *payload, uint8_t payload_length, uint8_t *out_buffer, uint8_t out_buffer_length);

int transact_frame(uint8_t address, uint8_t operation, uint8_t *payload, uint8_t payload_length, uint8_t *out_payload_buffer, uint8_t out_payload_buffer_length);

uint16_t calc_serial_number_crc16(uint8_t *data, uint8_t length);

#endif