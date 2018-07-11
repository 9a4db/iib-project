#ifndef USB_SERIAL_LINK_H
#define USB_SERIAL_LINK_H

#include "gps.h"
#include "packets.h"

/* Log Message Types */
#define MESSAGE_POS         0x01

/* Log Message */
typedef struct __attribute__((packed)) {

    uint8_t type;
    systime_t timestamp;
    uint8_t payload[123]; 
    
} packet_log;


/* Logging Functions */
void upload_position_packet(position_packet *pos_data);

/* Start USB Serial Thread */
void usb_serial_init(void);

/* Upload a log message */
void _upload_log(packet_log *packet);

#endif
