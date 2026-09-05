
#include "niu-packet.h"

#include <stdio.h>


int main() {
    uint8_t data_1[2] = {0x2d, 0x0f};
    uint8_t buffer[128];

    Packet packet;
    packet.address = 0x31;
    packet.operation = 2;
    packet.payload_data = data_1;
    packet.payload_count = 2;

    PacketDetector detector;
    packet_detect_init(&detector);
    Packet out_packet;
    uint8_t buffer2[128];
    packet_init(&out_packet, buffer2, 128);

    uint8_t len = packet_format_raw(&packet, buffer, 128);
    uint8_t have_packet = 0;
    for (uint8_t i = 0; i < len; ++i) {
        uint8_t b = buffer[i];
        printf(" %02X ", b);
        have_packet = packet_detect_push(&detector, &b, 1, &out_packet);
        printf("state: %d", detector.state);
        
    }

    printf("\n");

    if (have_packet) {
        printf("detected\n");
        printf("%02X %02X %02X\n", out_packet.address, out_packet.operation, out_packet.payload_count);
        for (uint8_t i = 0; i < out_packet.payload_count; ++i) {
            uint8_t b = out_packet.payload_data[i];
            printf("%02X ", b);
        }
    }
    printf("\n");
    


    printf("end\n");
    return 0;
}
