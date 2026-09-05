
#include "stm8s.h"

typedef struct Packet {
    uint8_t address;
    uint8_t operation;
    uint8_t *payload_data;
    uint8_t payload_count;
    uint8_t payload_max_count;
} Packet;


typedef enum DetectState {
    DETECT_EMPTY, DETECT_HEAD, DETECT_BODY
} DetectState;


typedef struct PacketDetector {
    DetectState state;
    uint8_t head_data[4];
    uint8_t head_index;
    uint8_t body_data[128];
    uint8_t body_index;
} PacketDetector;


void packet_init(Packet *packet, uint8_t *data, uint8_t max_size);

uint8_t packet_calc_size(Packet *packet);

uint8_t packet_format_raw(Packet *packet, uint8_t *buffer, uint8_t buffer_size);

void packet_detect_init(PacketDetector *detector);

uint8_t packet_detect_push(PacketDetector *detector, uint8_t *data, uint8_t data_size, Packet *out_packet);
