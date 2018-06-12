#ifndef PACKETS_H
#define PACKETS_H

/* Position Packet */
typedef struct __attribute__((packed)) {

    int32_t lon, lat;
    int32_t height;
    uint8_t num_sat;
    uint8_t fix_type;
    uint16_t year;
    uint8_t month, day, hour, minute, second;
    
} position_packet;

#endif
