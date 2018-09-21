#!/usr/bin/env python3

import sys
import struct
import serial
import datetime

# Useage
if len(sys.argv) != 2:
    print("Usage: {} /dev/ttyACMx".format(sys.argv[0]))
    sys.exit(1)

# Message Type Definitions          
MESSAGE_POSITION = 1
  
# Open Serial Port
ser = serial.Serial(sys.argv[1])
print("Listening on", ser.name)

# Fetch & Decode
while True:

    # Read in a Log
    data = ser.read(128)
      
    # Get Message Metadata
    meta_data = struct.unpack('<BI', data[0:5])
    log_type = meta_data[0]
    systick = meta_data[1]
    systick /= 10000.0
    
    
    # Handle Position Packet
    if (log_type == MESSAGE_POSITION):
        payload = data[5:27]
        pos = struct.unpack('<iiiBBhBBBBB?', payload)
        print("POSITION INFO:")
        print("Timestamp   ", systick, " s")
        print("Longitude   ", (pos[0]/10000000), "degrees")
        print("Latitude    ", (pos[1]/10000000), "degrees")
        print("Height      ", (pos[2]/1000), "m")  
        print("Satellites  ", pos[3])
        print("Fix Type    ", pos[4])
        print('Date         {:%Y-%m-%d %H:%M:%S}'.format(datetime.datetime(pos[5], pos[6], pos[7], pos[8], pos[9], pos[10])))
        if(pos[11]):
            print("PLL Status   Locked")
        else:
            print("PLL Status   Unlocked")
        print("\n\n\n\n\n\n\n\n\n\n\n")
 
